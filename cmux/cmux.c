/*
Copyright (c) 2020, Laird Connectivity
Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
*/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <termios.h>
#include <linux/gsmmux.h>
#include <linux/tty.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/sysmacros.h>
#include <signal.h>
#include <stdlib.h>

//For the moment, cmux won't send any stop command to modem when exits.

//Not defined in <linux/gsmmux.h>
#ifndef GSMIOC_ENABLE_NET
#define GSMIOC_ENABLE_NET      _IOW('G', 2, struct gsm_netconfig)
#define GSMIOC_DISABLE_NET     _IO('G', 3)
#endif

#define SERIAL_PORT	"/dev/ttyS4"
#define MUXED_AT_CMD_SERIAL_PORT "/dev/gsmtty3"
#define DEFAULT_SPEED B3000000	//default baudrate
#define MTU 1428
#define MODEM_RESET "at+cfun=15\n"

// terminate frame from section 6 of
// https://www.kernel.org/doc/Documentation/serial/n_gsm.txt
static const uint8_t gsm0710_terminate[] = {
       0xf9, /* open flag */
       0x03, /* channel 0 */
       0xef, /* UIH frame */
       0x05, /* 2 data bytes */
       0xc3, /* terminate 1 */
       0x01, /* terminate 2 */
       0xf2, /* crc */
       0xf9, /* close flag */
};

void signal_handler(int signum) {

    printf("cmux received signal: %d\n", signum);

}

int main(void) {
	struct termios tty;
	int ldisc = N_GSM0710, len;
	struct gsm_config gsm;
	char atbuf[64];
	int fd=0;
	int muxed_fd=0;

	/* open the serial port connected to the modem */
	fd = open(SERIAL_PORT, O_RDWR | O_NOCTTY | O_NDELAY);
	if (-1 == fd) {
		fprintf(stderr, "Cannot open " SERIAL_PORT " : %s\n", strerror(errno));
		return -1;
	}

	/* configure the serial port : speed, flow control ... */
	if (-1 == tcgetattr(fd, &tty)) {
		fprintf(stderr, "tcgetattr : %s\n", strerror(errno));
		close(fd);
		return -1;
	}

	tty.c_iflag = tty.c_oflag = tty.c_lflag = 0;
	tty.c_cflag = CS8 | CREAD | CLOCAL | CRTSCTS;

	cfsetospeed(&tty, DEFAULT_SPEED);
	cfsetispeed(&tty, DEFAULT_SPEED);

	tty.c_cc[VMIN] = 1;
	tty.c_cc[VTIME] = 0;

	if (-1 == tcsetattr(fd, TCSANOW, &tty)) {
		fprintf(stderr, "tcsetattr : %s\n", strerror(errno));
		close(fd);
		return -1;
	}

	/* send the AT commands to switch the modem to CMUX mode
	   and check that it's successful (should return OK) */
	len = snprintf(atbuf, sizeof(atbuf), "AT+CMUX=0,0,,%d\r\n", MTU);
	if (-1 == write(fd, atbuf, len)) {
		fprintf(stderr, "write " SERIAL_PORT " : %s\n", strerror(errno));
		close(fd);
		return -1;
	}

	/* experience showed that some modems need some time before
	   being able to answer to the first MUX packet so a delay
	   may be needed here in some case */
	sleep(1);

	len = read(fd, atbuf, sizeof(atbuf) - 1);
	if (len < 0) {
		fprintf(stderr, "read " SERIAL_PORT " : %d: %s\n", errno, strerror(errno));
		close(fd);
		return -1;
	}
	atbuf[len] = 0;

	if (!strstr(atbuf, "OK")) {
		fprintf(stderr, "response bogus : %s\n", atbuf);
		close(fd);
		return -1;
	}

	/* use n_gsm line discipline */
	ioctl(fd, TIOCSETD, &ldisc);

	/* get n_gsm configuration */
	if (ioctl(fd, GSMIOC_GETCONF, &gsm) < 0) {
		fprintf(stderr, "ioctl GSMIOC_GETCONF : %s\n", strerror(errno));
		close(fd);
		return -1;
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
		close(fd);
		return -1;
	}

	/* wait to keep the line discipline enabled, wake it up with a signal */
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGHUP, signal_handler);
	signal(SIGPIPE, signal_handler);
	signal(SIGKILL, signal_handler);
	signal(SIGUSR1, signal_handler);

	pause();

	muxed_fd = open(MUXED_AT_CMD_SERIAL_PORT, O_RDWR | O_NOCTTY | O_NDELAY);

	if (muxed_fd > 0 )
	{
	    write(muxed_fd, MODEM_RESET, sizeof(MODEM_RESET));

	    close(muxed_fd);
	}
	else
	    printf("cmux - error opening %s: %s\n", MUXED_AT_CMD_SERIAL_PORT, strerror(errno));

	/* terminate gsm 0710 multiplexing on the modem side */
	len = write(fd, gsm0710_terminate, sizeof(gsm0710_terminate));
	if (len != sizeof(gsm0710_terminate))
	    fprintf(stderr, "Failed to terminate gsm multiplexing");

	printf("cmux daemon exit!\n");

	close(fd);
	return 0;
}
