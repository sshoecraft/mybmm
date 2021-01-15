
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <netdb.h>
#include "mybmm.h"

#define DEFAULT_PORT 23

struct ip_session {
	int fd;
	char target[MYBMM_TARGET_LEN+1];
	int port;
};
typedef struct ip_session ip_session_t;

static int ip_init(mybmm_config_t *conf) {
	return 0;
}

static void *ip_new(mybmm_config_t *conf, ...) {
	ip_session_t *s;
	va_list ap;
	char *target,*p;

	va_start(ap,conf);
	target = va_arg(ap,char *);
	va_end(ap);
	dprintf(5,"target: %s\n", target);

	s = calloc(1,sizeof(*s));
	if (!s) {
		perror("ip_new: malloc");
		return 0;
	}
	s->target[0] = 0;
	p = strchr(target,':');
	if (p) *p = 0;
	strncat(s->target,target,MYBMM_TARGET_LEN);
	if (p) {
		p++;
		s->port = atoi(p);
	}
	if (!s->port) s->port = DEFAULT_PORT;
	dprintf(5,"target: %s, port: %d\n", s->target, s->port);
	return s;
}

static int ip_open(void *handle) {
	ip_session_t *s = handle;
	struct sockaddr_in addr;
	socklen_t sin_size;
	struct hostent *he;
	struct timeval tv;
	char temp[MYBMM_TARGET_LEN];
	uint8_t *ptr;

	s->fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s->fd < 0) {
		perror("ip_open: socket");
		return 1;
	}

	/* Try to resolve the target */
	he = (struct hostent *) 0;
	if (!is_ip(s->target)) {
		he = gethostbyname(s->target);
		dprintf(6,"he: %p\n", he);
		if (he) {
			ptr = (unsigned char *) he->h_addr;
			sprintf(temp,"%d.%d.%d.%d",ptr[0],ptr[1],ptr[2],ptr[3]);
		}
	}
	if (!he) strcpy(temp,s->target);
	dprintf(3,"temp: %s\n",temp);

	memset(&addr,0,sizeof(addr));
	sin_size = sizeof(addr);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(temp);
	addr.sin_port = htons(s->port);
	if (connect(s->fd,(struct sockaddr *)&addr,sin_size) < 0) {
		printf("ip_open: connect to %s: %s\n", s->target, strerror(errno));
		goto ip_open_error;
	}

//	dprintf(1,"setting timeout\n");
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	if (setsockopt(s->fd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv)) < 0) {
		perror("socket_open: setsockopt SO_RCVTIMEO");
		goto ip_open_error;
	}

	return 0;
ip_open_error:
	close(s->fd);
	s->fd = -1;
	return 1;
}

static int ip_read(void *handle, ...) {
	ip_session_t *s = handle;
	uint8_t *buf,ch;
	int buflen, bytes, bidx, num;
	va_list ap;
	struct timeval tv;
	fd_set rdset;

	tv.tv_usec = 0;
	tv.tv_sec = 1;

	va_start(ap,handle);
	buf = va_arg(ap, uint8_t *);
	buflen = va_arg(ap, int);
	va_end(ap);

	dprintf(5,"buf: %p, buflen: %d\n", buf, buflen);

#if 1
	FD_ZERO(&rdset);
	dprintf(5,"reading...\n");
	bidx=0;
	while(1) {
		FD_SET(s->fd,&rdset);
		num = select(s->fd+1,&rdset,0,0,&tv);
		dprintf(5,"num: %d\n", num);
		if (!num) break;
		dprintf(5,"buf: %p, bufen: %d\n", buf, buflen);
		bytes = read(s->fd, buf, buflen);
		dprintf(5,"bytes: %d\n", bytes);
		if (bytes < 0) {
			bidx = -1;
			break;
		} else if (bytes == 0) {
			break;
		} else if (bytes > 0) {
			buf += bytes;
			bidx += bytes;
			buflen -= bytes;
			if (buflen < 1) break;
		}
	}

	if (debug >= 5) bindump("serial",buf,bidx);
	dprintf(5,"returning: %d\n", bidx);
	return bidx;
#else
	bidx=0;
	dprintf(5,"reading...\n");
	while(1) {
		bytes = read(s->fd, &ch, 1);
		dprintf(6,"bytes: %d\n", bytes);
		if (bytes < 0) {
			if (errno != EAGAIN) bidx = -1;
			break;
		} else if (bytes == 0) {
			break;
		} else if (bytes > 0) {
//			dprintf(6,"ch: %02x\n", ch);
			buf[bidx++] = ch;
			if (bidx >= buflen) break;
		}
//		usleep(100);
	}
//	bindump("ip",buf,bidx);

	dprintf(5,"returning: %d\n", bidx);
	return bidx;
#endif
}

static int ip_write(void *handle, ...) {
	ip_session_t *s = handle;
	uint8_t *buf;
	int bytes,buflen;
	va_list ap;

	va_start(ap,handle);
	buf = va_arg(ap, uint8_t *);
	buflen = va_arg(ap, int);
	va_end(ap);

	dprintf(5,"buf: %p, buflen: %d\n", buf, buflen);

	if (debug >= 3) bindump("TO BMS",buf,buflen);
	bytes = write(s->fd,buf,buflen);
	dprintf(5,"bytes: %d\n", bytes);
	usleep ((buflen + 25) * 100);
	return bytes;
}


static int ip_close(void *handle) {
	ip_session_t *s = handle;

	close(s->fd);
	return 0;
}

EXPORT_API mybmm_module_t ip_module = {
	MYBMM_MODTYPE_TRANSPORT,
	"ip",
	0,
	ip_init,
	ip_new,
	ip_open,
	ip_read,
	ip_write,
	ip_close
};
