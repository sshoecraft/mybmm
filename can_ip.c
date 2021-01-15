
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

int devserver_send(int fd, uint8_t opcode, uint8_t unit, void *data, int datasz);
int devserver_recv(int fd, uint8_t *opcode, uint8_t *unit, void *data, int datasz, int timeout);
int devserver_request(int fd, uint8_t opcode, uint8_t unit, void *data, int len);
int devserver_reply(int fd, uint8_t status, uint8_t unit ,uint8_t *data, int len);
int devserver_error(int fd, uint8_t status);

#define devserver_getshort(p) (uint16_t)(*(p) | (*((p)+1) << 8))
#define devserver_putshort(p,v) { float tmp; *(p) = ((uint16_t)(tmp = (v))); *((p)+1) = ((uint16_t)(tmp = (v)) >> 8); }
#define devserver_get16(p) (uint16_t)(*(p) | (*((p)+1) << 8))
#define devserver_put16(p,v) { float tmp; *(p) = ((uint16_t)(tmp = (v))); *((p)+1) = ((uint16_t)(tmp = (v)) >> 8); }
#define devserver_get32(p) (uint16_t)(*(p) | (*((p)+1) << 8) | (*((p)+2) << 16) | (*((p)+3) << 24))
#define devserver_put32(p,v) *(p) = ((int)(v) & 0xFF); *((p)+1) = ((int)(v) >> 8) & 0xFF; *((p)+2) = ((int)(v) >> 16) & 0xFF; *((p)+3) = ((int)(v) >> 24) & 0xFF

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

#define PACKET_START	0xD5
#define PACKET_END	0xEB
#ifdef CHKSUM
#define PACKET_MIN	7
#else
#define PACKET_MIN	5
#endif

struct devserver_header {
	uint8_t start;
	uint8_t opcode;
	uint8_t unit;
	uint8_t len;
};
typedef struct devserver_header devserver_header_t;

#ifdef CHKSUM
static uint16_t chksum(uint8_t *data, uint16_t datasz) {
	uint16_t sum;
	register int i;

	sum = 0;
	for(i=0; i < datasz/2; i += 2) sum += (data[i] | data[i] << 8);
	if (i < datasz) sum += data[i];
	return sum;
}
#endif

int devserver_send(int fd, uint8_t opcode, uint8_t unit, void *data, int datasz) {
	uint8_t pkt[256];
	int i,bytes;

	dprintf(5,"fd: %d, opcode: %d, unit: %d, data: %p, datasz: %d\n", fd, opcode, unit, data, datasz);
	if (fd < 0) return -1;

	i = 0;
	pkt[i++] = PACKET_START;
	pkt[i++] = opcode;
	pkt[i++] = unit;
	pkt[i++] = datasz;
//	devserver_put32(&pkt[i],datasz);
//	i += 2;
	if (data && datasz) {
		memcpy(&pkt[i],data,datasz);
		i += datasz;
	}
	pkt[i++] = PACKET_END;

	if (debug >= 5) bindump("<<<",pkt,i);
	bytes = write(fd, pkt, i);
	dprintf(5,"bytes: %d\n", bytes);
	return bytes;
}

int devserver_recv(int fd, uint8_t *opcode, uint8_t *unit, void *data, int datasz, int timeout) {
	uint8_t ch,len;
	int bytes,bytes_left;
	uint8_t pkt[256],i;

	/* Read the start */
	do {
		bytes = read(fd,&ch,1);
		dprintf(5,"bytes: %d, start: %02x\n", bytes, ch);
		if (bytes < 0) {
			return -1;
		} else if (bytes == 0)
			return 0;
	} while(ch != PACKET_START);
	i=0;
	pkt[i++] = ch;

	/* Read opcode */
	bytes = read(fd,opcode,1);
	dprintf(5,"bytes: %d, opcode: %d\n", bytes, *opcode);
	if (bytes < 1) return -1;
	pkt[i++] = *opcode;

	/* Read unit */
	bytes = read(fd,unit,1);
	dprintf(5,"bytes: %d, unit: %d\n", bytes, *unit);
	if (bytes < 1) return -1;
	pkt[i++] = *unit;

	/* Read length */
	bytes = read(fd,&len,1);
	dprintf(5,"bytes: %d, len: %d\n", bytes, len);
	if (bytes < 1) return -1;
	pkt[i++] = len;

	/* Read the data */
	dprintf(5,"data: %p, datasz: %d\n", data, datasz);
	bytes_left = len;
	if (data && datasz && len) {
		if (len > datasz) len = datasz;
		bytes = read(fd,data,len);
		dprintf(5,"bytes: %d\n", bytes);
		if (bytes < 1) return -1;
		bytes_left -= len;
		memcpy(&pkt[i],data,len);
		i += len;
	}
	/* Drain any remaining data */
	dprintf(5,"bytes_left: %d\n", bytes_left);
	while(bytes_left--) {
		read(fd,&ch,1);
		pkt[i++] = ch;
	}

	/* Read the end mark */
	bytes = read(fd,&ch,1);
	dprintf(5,"bytes: %d, ch: %02x\n", bytes, ch);
	if (bytes < 1) return -1;
	if (ch != PACKET_END) return -1;
	pkt[i++] = ch;

	dprintf(5,"returning: %d\n", len);
	if (debug >= 5) bindump(">>>",pkt,i);
	return len;
}

int devserver_request(int fd, uint8_t opcode, uint8_t unit, void *data, int len) {
	uint8_t status,runit;
	int bytes;

	/* Send the req */
	dprintf(1,"sending req\n");
	bytes = devserver_send(fd,opcode,unit,0,0);
	dprintf(1,"bytes: %d\n", bytes);
	if (bytes < 0) return -1;

	/* Read the response */
	bytes = devserver_recv(fd,&status,&runit,data,len,1000);
	dprintf(1,"bytes: %d, status: %d, runit: %d\n", bytes, status, runit);
	if (bytes < 0) return -1;
	if (status != 0) return 0;
	return bytes;
}

int devserver_reply(int fd, uint8_t status, uint8_t unit, uint8_t *data, int len) {
	int bytes;

	bytes = devserver_send(fd,status,unit,data,len);
	dprintf(1,"bytes: %d\n", bytes);
	return (bytes < 0 ? 1 : 0);
}

int devserver_error(int fd, uint8_t code) {
	int bytes;

	bytes = devserver_send(fd,code,0,0,0);
	dprintf(1,"bytes: %d\n", bytes);
	return (bytes < 0 ? 1 : 0);
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
//	int bytes;

	devserver_request(s->fd,CANSERVER_CLOSE,s->unit,0,0);
//	dprintf(5,"bytes: %d\n", bytes);
	close(s->fd);
	s->fd = -1;
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
