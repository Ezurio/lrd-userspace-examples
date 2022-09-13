/*
 * Copyright (c) 2020, Laird Connectivity
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <termios.h>
#include <linux/gsmmux.h>
#include <linux/tty.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/sysmacros.h>
#include <signal.h>
#include <stdlib.h>
#include <poll.h>

//For the moment, cmux won't send any stop command to modem when exits.

//Not defined in <linux/gsmmux.h>
#ifndef GSMIOC_ENABLE_NET
#define GSMIOC_ENABLE_NET      _IOW('G', 2, struct gsm_netconfig)
#define GSMIOC_DISABLE_NET     _IO('G', 3)
#endif

#define SERIAL_PORT     "/dev/ttyS4"
#define MUXED_AT_CMD_SERIAL_PORT "/dev/gsmtty3"
#define DEFAULT_SPEED B3000000  //default baudrate
#define MTU 1428
#define MODEM_RESET "at+cfun=15\n"

// terminate frame from section 6 of
// https://www.kernel.org/doc/Documentation/serial/n_gsm.txt
static const uint8_t gsm0710_terminate[] = {
	0xf9,   /* open flag */
	0x03,   /* channel 0 */
	0xef,   /* UIH frame */
	0x05,   /* 2 data bytes */
	0xc3,   /* terminate 1 */
	0x01,   /* terminate 2 */
	0xf2,   /* crc */
	0xf9,   /* close flag */
};

void signal_handler(int signum)
{
	printf("cmux received signal: %d\n", signum);
}

enum eAction {
	eContinue,
	eSuccess,
	eAbort,
};

static enum eAction response_process(const char *buf)
{
	/* Command successful */
	if (!strcmp(buf, "OK"))
		return eSuccess;

	/* Command failed */
	if (!strncmp(buf, "ERROR", 5)) {
		fprintf(stderr, "read " SERIAL_PORT " : %s\n", "Modem refused to enter CMUX mode");
		return eAbort;
	}

	/* Got something else like URN, echo, or textual response */
	return eContinue;
}

static enum eAction modem_response_loop(int fd, enum eAction (*rspProc)(const char *buf))
{
	struct pollfd fds;
	char *ch, *chr;
	int flags, pollrc, len, off = 0;
	enum eAction rc = eAbort;
	char buf[64];

	/* Change file descriptor to non blocking */
	flags = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);

	fds.fd = fd;
	fds.events = POLLIN;
	buf[0] = 0;

	for (;;) {
		/* check if there are more responses in the buffer, sometimes we get multiple at once */
		ch = strchr(buf, '\n');
		if (ch) {
			*ch = 0;

			/* Trim CR from the string end */
			chr = ch;
			while (--chr >= buf && *chr == '\r')
				*chr = 0;

			/* check string responses */
			rc = rspProc(buf);
			if (rc != eContinue)
				break;

			/* shift buffer to remove processed portion */
			off -= ch - buf - 1;
			memmove(buf, ch + 1, off + 1);
			continue;
		}

		if (off >= sizeof(buf) - 1) {
			/* buffer is full of something with no LF  */
			fprintf(stderr, "read " SERIAL_PORT " : %s\n", "Receive buffer overflow");
			break;
		}

		pollrc = poll(&fds, 1, 2000);
		if (pollrc < 0) {
			/* poll failed or aborted by signal */
			fprintf(stderr, "poll " SERIAL_PORT " : %s\n", strerror(errno));
			break;
		}
		if (poll == 0) {
			/* poll timed out */
			fprintf(stderr, "poll " SERIAL_PORT " : Modem does not reply\n");
			break;
		}

		if (fds.revents & POLLIN) {
			/* have data on the serial port so read them all */
			len = read(fd, buf + off, sizeof(buf) - 1 - off);
			if (len < 0) {
				fprintf(stderr, "read " SERIAL_PORT " : %d: %s\n", errno, strerror(errno));
				break;
			}
			off += len;
			buf[off] = 0;
		}
	}

	/* Change file descriptor back to blocking */
	fcntl(fd, F_SETFL, flags);

	return rc;
}

int main(void)
{
	struct termios tty;
	struct gsm_config gsm;
	int ldisc = N_GSM0710, len, rc = -1;
	int fd, muxed_fd;
	char buf[64];

	/* open the serial port connected to the modem */
	fd = open(SERIAL_PORT, O_RDWR | O_NOCTTY | O_NDELAY);
	if (-1 == fd) {
		fprintf(stderr, "Cannot open " SERIAL_PORT " : %s\n", strerror(errno));
		return -1;
	}

	/* configure the serial port : speed, flow control ... */
	if (-1 == tcgetattr(fd, &tty)) {
		fprintf(stderr, "tcgetattr : %s\n", strerror(errno));
		goto exit;
	}

	tty.c_iflag = tty.c_oflag = tty.c_lflag = 0;
	tty.c_cflag = CS8 | CREAD | CLOCAL | CRTSCTS;

	cfsetospeed(&tty, DEFAULT_SPEED);
	cfsetispeed(&tty, DEFAULT_SPEED);

	tty.c_cc[VMIN] = 1;
	tty.c_cc[VTIME] = 0;

	if (-1 == tcsetattr(fd, TCSANOW, &tty)) {
		fprintf(stderr, "tcsetattr : %s\n", strerror(errno));
		goto exit;
	}

	/* send the AT commands to switch the modem to CMUX mode
	 * and check that it's successful (should return OK) */
	len = snprintf(buf, sizeof(buf), "AT+CMUX=0,0,,%u\r\n", MTU);
	if (-1 == write(fd, buf, len)) {
		fprintf(stderr, "write " SERIAL_PORT " : %s\n", strerror(errno));
		goto exit;
	}

	if (modem_response_loop(fd, response_process) == eAbort)
		goto exit;

	/* use n_gsm line discipline */
	if (ioctl(fd, TIOCSETD, &ldisc) < 0) {
		fprintf(stderr, "ioctl TIOCSETD : %s\n", strerror(errno));
		goto exit;
	}

	/* get n_gsm configuration */
	if (ioctl(fd, GSMIOC_GETCONF, &gsm) < 0) {
		fprintf(stderr, "ioctl GSMIOC_GETCONF : %s\n", strerror(errno));
		goto exit;
	}

	/* we are initiator and need encoding 0 (basic) */
	gsm.initiator = 1;
	gsm.encapsulation = 0;

	gsm.mru = MTU;
	gsm.mtu = MTU;
	gsm.t1 = 10;
	gsm.n2 = 3;
	gsm.t2 = 30;
	gsm.t3 = 10;
	//gsm.k = 0;

	/* set the new configuration */
	if (ioctl(fd, GSMIOC_SETCONF, &gsm) < 0) {
		fprintf(stderr, "ioctl GSMIOC_SETCONF : %s\n", strerror(errno));
		goto exit;
	}

	/* wait to keep the line discipline enabled, wake it up with a signal */
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGHUP, signal_handler);
	signal(SIGPIPE, signal_handler);
	signal(SIGKILL, signal_handler);
	signal(SIGUSR1, signal_handler);

	pause();

	rc = 0;

	muxed_fd = open(MUXED_AT_CMD_SERIAL_PORT, O_RDWR | O_NOCTTY | O_NDELAY);
	if (muxed_fd > 0) {
		if (write(muxed_fd, MODEM_RESET, sizeof(MODEM_RESET)) != sizeof(MODEM_RESET))
			fprintf(stderr, "Failed to terminate gsm multiplexing");
		close(muxed_fd);
	} else
		printf("cmux - error opening %s: %s\n", MUXED_AT_CMD_SERIAL_PORT, strerror(errno));

	/* terminate gsm 0710 multiplexing on the modem side */
	len = write(fd, gsm0710_terminate, sizeof(gsm0710_terminate));
	if (len != sizeof(gsm0710_terminate))
		fprintf(stderr, "Failed to terminate gsm multiplexing");

	printf("cmux daemon exit!\n");

exit:
	close(fd);
	return rc;
}
