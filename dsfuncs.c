
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#if 0
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
#endif
#include "utils.h"
#include "dsfuncs.h"
#include "debug.h"

#define PACKET_START	0xD5
#define PACKET_END	0xEB
#ifdef CHKSUM
#define PACKET_MIN	7
#else
#define PACKET_MIN	5
#endif

#if 0
struct devserver_header {
	uint8_t start;
	uint8_t opcode;
	uint8_t unit;
	uint8_t len;
};
typedef struct devserver_header devserver_header_t;
#endif

#define _getshort(p) (uint16_t)(*(p) | (*((p)+1) << 8))

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
#ifdef CHKSUM
	devserver_putshort(&pkt[i],chksum(data,datasz));
	i += 2;
#endif
	pkt[i++] = PACKET_END;

	if (debug >= 5) bindump("<<<",pkt,i);
	bytes = write(fd, pkt, i);
	dprintf(5,"bytes: %d\n", bytes);
	return bytes;
}

int devserver_recv(int fd, uint8_t *opcode, uint8_t *unit, void *data, int datasz, int timeout) {
	uint8_t ch,len;
	int i,bytes,bytes_left;
	uint8_t pkt[256];

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

#ifdef CHKSUM
	{
		uint16_t sum,psum;
		char temp[2];

		/* Read the checksum */
		bytes = read(fd,&temp,2);
		dprintf(5,"bytes: %d\n", bytes);
		if (bytes < 1) return -1;
		psum = _getshort(temp);
		sum = chksum(data,len);
		dprintf(5,"sum: %02x, psum: %02x\n", sum, psum);
		pkt[i++] = temp[0];
		pkt[i++] = temp[1];
	}
#endif

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
