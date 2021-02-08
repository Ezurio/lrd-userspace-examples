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
#define DEFAULT_SPEED B3000000	//default baudrate
#define MTU 1428

void signal_handler(int signum) {
	printf("cmux daemon exit!");
	exit(0);
}

int main(void) {

	int fd, major;
	struct termios tty;
	int ldisc = N_GSM0710;
	struct gsm_config gsm;
	char atbuf[64];

	/* open the serial port connected to the modem */
	fd = open(SERIAL_PORT, O_RDWR | O_NOCTTY | O_NDELAY);
	if ( -1 == fd){
		return -1;
	}

	/* configure the serial port : speed, flow control ... */
	if (-1 == tcgetattr(fd, &tty)){
		close(fd);
		return -1;
	}

	tty.c_iflag = tty.c_oflag = tty.c_lflag = 0;
	tty.c_cflag = CS8 | CREAD | CLOCAL | CRTSCTS;

	cfsetospeed(&tty, DEFAULT_SPEED);
	cfsetispeed(&tty, DEFAULT_SPEED);

	tty.c_cc[VMIN] = 1;
	tty.c_cc[VTIME] = 0;

	if ( -1 == tcsetattr(fd, TCSANOW, &tty)){
		close(fd);
		return -1;
	}

	/* send the AT commands to switch the modem to CMUX mode
	   and check that it's successful (should return OK) */
	snprintf(atbuf, sizeof(atbuf), "AT+CMUX=0,0,,%d\r\n", MTU);
	if (-1 == write(fd, atbuf, strlen(atbuf))){
		close(fd);
		return -1;
	}

	/* experience showed that some modems need some time before
	   being able to answer to the first MUX packet so a delay
	   may be needed here in some case */
	sleep(1);

	memset(atbuf, 0, sizeof(atbuf));
	if(read(fd, atbuf, sizeof(atbuf)) < 1){
		close(fd);
		return -1;
	}

	if(!strstr(atbuf, "OK")){
		close(fd);
		return -1;
	}

	/* use n_gsm line discipline */
	ioctl(fd, TIOCSETD, &ldisc);

	/* get n_gsm configuration */
	if (ioctl(fd, GSMIOC_GETCONF, &gsm) < 0){
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
	if (ioctl(fd, GSMIOC_SETCONF, &gsm) < 0){
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

	close(fd);
	return 0;
}
