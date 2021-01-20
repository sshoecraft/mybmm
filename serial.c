
#include <stdio.h>
#include <errno.h>
#include <fcntl.h> 
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include "mybmm.h"

#define DEFAULT_SPEED 9600

struct serial_session {
	int fd;
	char device[MYBMM_TARGET_LEN+1];
	int speed,data,stop,parity;
};
typedef struct serial_session serial_session_t;

static int serial_init(mybmm_config_t *conf) {
	return 0;
}

static void *serial_new(mybmm_config_t *conf, ...) {
	serial_session_t *s;
	char device[MYBMM_TARGET_LEN+1],*p;
	int baud,parity,stop;
	va_list(ap);
	char *target;

	va_start(ap,conf);
	target = va_arg(ap,char *);
	va_end(ap);
	dprintf(1,"target: %s\n", target);

	p = strchr(target,',');
	if (p) {
		*p++ = 0;
		baud = atoi(p);
	} else {
		baud = DEFAULT_SPEED;
	}
	device[0] = 0;
	strncat(device,target,MYBMM_TARGET_LEN);
	stop=8;
	parity=1;
	dprintf(1,"baud: %d, stop: %d, parity: %d\n", baud, stop, parity);

	s = calloc(1,sizeof(*s));
	if (!s) {
		perror("serial_new: malloc");
		return 0;
	}
	s->fd = -1;
	strcpy(s->device,device);
	s->speed = baud;
	s->stop = stop;
	s->parity = parity;

	return s;
}

static int set_interface_attribs (int fd, int speed, int data, int parity, int stop, int vmin, int vtime) {
        struct termios tty;

        if (tcgetattr (fd, &tty) != 0) {
                printf("error %d from tcgetattr\n", errno);
                return -1;
        }

	tty.c_cflag &= ~CBAUD;
	tty.c_cflag |= CBAUDEX;
//	tty.c_ispeed = speed;
//	tty.c_ospeed = speed;
	cfsetspeed (&tty, speed);

	switch(data) {
	case 5:
        	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS5;
		break;
	case 6:
        	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS6;
		break;
	case 7:
        	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS7;
		break;
	case 8:
	default:
        	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
		break;
	}
        // disable IGNBRK for mismatched speed tests; otherwise receive break
        // as \000 chars
        tty.c_iflag &= ~IGNBRK;         // disable break processing
        tty.c_lflag = 0;                // no signaling chars, no echo,
                                        // no canonical processing
        tty.c_oflag = 0;                // no remapping, no delays
        tty.c_cc[VMIN]  = vmin;		// min num of chars before returning
        tty.c_cc[VTIME] = vtime;	// useconds to wait for a char

        tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

        tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
                                        // enable reading
        tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
        tty.c_cflag |= parity;
        tty.c_cflag &= ~CSTOPB;		/* No stop bits */
        tty.c_cflag |= stop;
        tty.c_cflag &= ~CRTSCTS; /* No CTS */

        if (tcsetattr (fd, TCSANOW, &tty) != 0) {
		printf("error %d from tcsetattr\n", errno);
                return -1;
        }
        return 0;
}

static int serial_open(void *handle) {
	serial_session_t *s = handle;
	char path[256];

	if (s->fd >= 0) return 0;

	strcpy(path,s->device);
	if ((access(path,0)) == 0) {
		if (strncmp(path,"/dev",4) != 0) {
			sprintf(path,"/dev/%s",s->device);
			dprintf(1,"path: %s\n", path);
		}
	}
	s->fd = open(path, O_RDWR | O_NOCTTY | O_SYNC);
	if (s->fd < 0) {
		printf("error %d opening %s: %s", errno, path, strerror(errno));
		return 1;
	}
	set_interface_attribs(s->fd, s->speed, s->data, s->parity, s->stop, 1, 5);

	return 0;
}

static int serial_read(void *handle, ...) {
	serial_session_t *s = handle;
	uint8_t *buf, *p;
	int buflen, bytes, bidx, num;
	struct timeval tv;
	va_list ap;
	fd_set rdset;

	if (s->fd < 0) return -1;

	tv.tv_usec = 0;
	tv.tv_sec = 1;

	va_start(ap,handle);
	buf = va_arg(ap, uint8_t *);
	buflen = va_arg(ap, int);
	va_end(ap);
	dprintf(1,"buf: %p, buflen: %d\n", buf, buflen);

	FD_ZERO(&rdset);
	dprintf(5,"reading...\n");
	bidx=0;
	p = buf;
	while(1) {
		FD_SET(s->fd,&rdset);
		num = select(s->fd+1,&rdset,0,0,&tv);
		dprintf(5,"num: %d\n", num);
		if (!num) break;
		dprintf(5,"p: %p, bufen: %d\n", p, buflen);
		bytes = read(s->fd, p, buflen);
		dprintf(5,"bytes: %d\n", bytes);
		if (bytes < 0) {
			bidx = -1;
			break;
		} else if (bytes == 0) {
			break;
		} else if (bytes > 0) {
			p += bytes;
			bidx += bytes;
			buflen -= bytes;
			if (buflen < 1) break;
		}
	}

	dprintf(1,"debug: %d\n",debug);
	if (debug >= 5) bindump("serial",buf,bidx);
	dprintf(1,"returning: %d\n", bidx);
	return bidx;
}

static int serial_write(void *handle, ...) {
	serial_session_t *s = handle;
	uint8_t *buf;
	int bytes,buflen;
	va_list ap;

	if (s->fd < 0) return -1;

	va_start(ap,handle);
	buf = va_arg(ap, uint8_t *);
	buflen = va_arg(ap, int);
	va_end(ap);

	dprintf(1,"writing...\n");
	bytes = write(s->fd,buf,buflen);
	dprintf(1,"bytes: %d\n", bytes);
	usleep ((buflen + 25) * 100);
	return bytes;
}

static int serial_close(void *handle) {
	serial_session_t *s = handle;

	if (s->fd >= 0) {
		close(s->fd);
		s->fd = -1;
	}
	return 0;
}

mybmm_module_t serial_module = {
	MYBMM_MODTYPE_TRANSPORT,
	"serial",
	0,
	serial_init,
	serial_new,
	serial_open,
	serial_read,
	serial_write,
	serial_close
};
