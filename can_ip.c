
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/can.h>
#include <ctype.h>
#include <netdb.h>
#include "mybmm.h"
#include "dsfuncs.h"

#define DEFAULT_PORT 3930
#define DEFAULT_BITRATE 250000

enum CANSERVER {
        CANSERVER_SUCCESS,
        CANSERVER_ERROR,
        CANSERVER_OPEN,
        CANSERVER_READ,
        CANSERVER_WRITE,
        CANSERVER_CLOSE,
        CANSERVER_TIMEOUT,
        CANSERVER_FILTER,
        CANSERVER_UP,
        CANSERVER_DOWN,
        CANSERVER_GETBR,
        CANSERVER_SETBR,
        CANSERVER_SETBT,
        CANSERVER_BCM,
};

struct can_ip_session {
	int fd;
	char target[MYBMM_TARGET_LEN+1];
	int port;
	char interface[16];
	int bitrate;
	uint8_t unit;
};
typedef struct can_ip_session can_ip_session_t;

static int can_ip_init(mybmm_config_t *conf) {
	return 0;
}

static void *can_ip_new(mybmm_config_t *conf, ...) {
	can_ip_session_t *s;
	va_list ap;
	char *target,*p;

	va_start(ap,conf);
	target = va_arg(ap,char *);
	va_end(ap);
	dprintf(5,"target: %s\n", target);

	s = calloc(1,sizeof(*s));
	if (!s) {
		perror("can_ip_new: malloc");
		return 0;
	}
	s->fd = -1;

	/* Format is addr:port:interface:speed */
	s->target[0] = 0;
	strncat(s->target,strele(0,",",target),sizeof(s->target)-1);
	p = strele(1,",",target);
	if (strlen(p)) s->port = atoi(p);
	else s->port = DEFAULT_PORT;
	p =strele(2,",",target);
	if (!p) {
		printf("error: can_ip requires ip,port,interface,speed\n");
		return 0;
	}
	s->interface[0] = 0;
	strncat(s->interface,p,sizeof(s->interface)-1);
	p = strele(3,",",target);
	if (strlen(p)) s->bitrate = atoi(p);
	else s->bitrate = DEFAULT_BITRATE;

	dprintf(1,"target: %s, port: %d, interface: %s, bitrate: %d\n", s->target, s->port, s->interface, s->bitrate);
	return s;
}

static int can_ip_open(void *handle) {
	can_ip_session_t *s = handle;
	struct sockaddr_in addr;
	socklen_t sin_size;
	struct hostent *he;
	struct timeval tv;
	uint8_t status;
	int bytes;
	char temp[MYBMM_TARGET_LEN];
	uint8_t *ptr;

	if (s->fd >= 0) return 0;

	dprintf(1,"Creating socket...\n");
	s->fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s->fd < 0) {
		perror("can_ip_open: socket");
		return 1;
	}

	/* Try to resolve the target */
	dprintf(1,"checking..\n");
	he = (struct hostent *) 0;
	if (!is_ip(s->target)) {
		he = gethostbyname(s->target);
		dprintf(1,"he: %p\n", he);
		if (he) {
			ptr = (unsigned char *) he->h_addr;
			sprintf(temp,"%d.%d.%d.%d",ptr[0],ptr[1],ptr[2],ptr[3]);
		}
	}
	if (!he) strcpy(temp,s->target);
	dprintf(1,"temp: %s\n",temp);

	dprintf(1,"Connecting to: %s\n",temp);
	memset(&addr,0,sizeof(addr));
	sin_size = sizeof(addr);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(temp);
	addr.sin_port = htons(s->port);
	if (connect(s->fd,(struct sockaddr *)&addr,sin_size) < 0) {
		perror("can_ip_open: connect");
		return 1;
	}

	dprintf(1,"setting timeout\n");
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	if (setsockopt(s->fd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv)) < 0) {
		perror("socket_open: setsockopt SO_SNDTIMEO");
		return 1;
	}

	dprintf(1,"sending open\n");
	bytes = devserver_send(s->fd,CANSERVER_OPEN,0,s->interface,strlen(s->interface)+1);
	dprintf(1,"bytes: %d\n", bytes);

	usleep (2500);

	/* Read the reply */
	bytes = devserver_recv(s->fd,&status,&s->unit,0,0,0);
	if (bytes < 0) return -1;
	dprintf(1,"bytes: %d, status: %d, unit: %d\n", bytes, status, s->unit);

	return 0;
}

static int can_ip_read(void *handle, ...) {
	can_ip_session_t *s = handle;
	uint8_t *buf,data[2],status,runit;
	int id, buflen, bytes, len;
	struct can_frame frame;
	va_list ap;

	if (s->fd < 0) return -1;

	va_start(ap,handle);
	id = va_arg(ap, int);
	buf = va_arg(ap, uint8_t *);
	buflen = va_arg(ap, int);
	va_end(ap);

	dprintf(4,"id: %03x, buf: %p, buflen: %d\n", id, buf, buflen);

	/* Send the read command */
	devserver_putshort(data,id);
	devserver_send(s->fd,CANSERVER_READ,s->unit,data,2);

	/* Read the response */
	bytes = devserver_recv(s->fd,&status,&runit,&frame,sizeof(frame),1000);
	dprintf(4,"bytes: %d, status: %d, runit: %d\n", bytes, status, runit);
	if (bytes < 0) return -1;
	/* If the id is 0xFFFF, return the whole frame */
	if (id == 0x0FFFF) {
		dprintf(8,"returning frame...\n");
		len = buflen < sizeof(frame) ? buflen : sizeof(frame);
		memcpy(buf,&frame,len);
	} else {
		len = buflen > frame.can_dlc ? frame.can_dlc : buflen;
		dprintf(8,"len: %d, can_dlc: %d\n", len, frame.can_dlc);
		memcpy(buf,&frame.data,len);
	}
	if (bytes > 0 && debug >= 8) bindump("FROM BMS",buf,buflen);
	dprintf(6,"returning: %d\n", bytes);
	return len;
}

static int can_ip_write(void *handle, ...) {
	can_ip_session_t *s = handle;
	uint8_t *buf,status,runit;
	int id,bytes,buflen;
	struct can_frame frame;
	va_list ap;

	if (s->fd < 0) return -1;

	va_start(ap,handle);
	id = va_arg(ap, int);
	buf = va_arg(ap, uint8_t *);
	buflen = va_arg(ap, int);
	va_end(ap);

	dprintf(5,"id: %03x, buf: %p, buflen: %d\n", id, buf, buflen);

	memset(&frame,0,sizeof(frame));
	frame.can_id = id;
	if (buflen > 8) buflen = 8;
	frame.can_dlc = buflen;
	memcpy(&frame.data,buf,buflen);

	bytes = devserver_send(s->fd,CANSERVER_WRITE,s->unit,&frame,sizeof(frame));
	dprintf(5,"bytes: %d\n", bytes);
	if (bytes < 0) return -1;

	/* Read the response */
	bytes = devserver_recv(s->fd,&status,&runit,0,0,0);
	dprintf(5,"bytes: %d, status: %d, runit: %d\n", bytes, status, runit);
	if (bytes < 0) return -1;
	return bytes;

	dprintf(5,"returning: %d\n", bytes);
	return bytes;
}


static int can_ip_close(void *handle) {
	can_ip_session_t *s = handle;

	if (s->fd >= 0) {
		devserver_request(s->fd,CANSERVER_CLOSE,s->unit,0,0);
		close(s->fd);
		s->fd = -1;
	}
	return 0;
}

mybmm_module_t can_ip_module = {
	MYBMM_MODTYPE_TRANSPORT,
	"can_ip",
	0,
	can_ip_init,
	can_ip_new,
	can_ip_open,
	can_ip_read,
	can_ip_write,
	can_ip_close
};
