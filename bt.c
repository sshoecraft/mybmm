/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.

Bluetooth transport

This was tested against the MLT-BT05 TTL-to-Bluetooth module (HM-10 compat) on the Pi3B+

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "gattlib.h"
#include "mybmm.h"

struct bt_session {
	gatt_connection_t *c;
	uuid_t uuid;
	char target[32];
	char data[4096];
	int len;
	int cbcnt;
};
typedef struct bt_session bt_session_t;

static int bt_init(mybmm_config_t *conf) {
	return 0;
}

static void *bt_new(mybmm_config_t *conf, ...) {
	bt_session_t *s;
	va_list ap;
	char *target;

	va_start(ap,conf);
	target = va_arg(ap,char *);
	va_end(ap);
	dprintf(5,"target: %s\n", target);

	s = calloc(1,sizeof(*s));
	if (!s) {
		perror("bt_new: malloc");
		return 0;
	}
	s->c = 0;
	strcpy(s->target,target);
	return s;
}

static void notification_cb(const uuid_t* uuid, const uint8_t* data, size_t data_length, void* user_data) {
	bt_session_t *s = (bt_session_t *) user_data;

	/* Really should check for overrun here */
	dprintf(7,"s->len: %d, data: %p, data_length: %d\n", s->len, data, data_length);
	if (s->len + data_length > sizeof(s->data)) data_length = sizeof(s->data) - s->len;
	memcpy(&s->data[s->len],data,data_length);
	s->len+=data_length;
	dprintf(7,"s->len: %d\n", s->len);
	s->cbcnt++;
	dprintf(7,"s->cbcnt: %d\n", s->cbcnt);
}

static int bt_open(void *handle) {
	bt_session_t *s = handle;
	uint16_t on = 0x0001;

#define UUID "0xffe1"
	if (gattlib_string_to_uuid(UUID, strlen(UUID)+1, &s->uuid) < 0) {
		fprintf(stderr, "Fail to convert string to UUID\n");
		return 1;
	}

	dprintf(1,"connecting to: %s\n", s->target);
	s->c = gattlib_connect(NULL, s->target, GATTLIB_CONNECTION_OPTIONS_LEGACY_DEFAULT);
	if (!s->c) {
		fprintf(stderr, "Fail to connect to the bluetooth device.\n");
		return 1;
	}
	dprintf(1,"s->c: %p\n", s->c);

	/* yes, its hardcoded. deal. */
	gattlib_write_char_by_handle(s->c, 0x0026, &on, sizeof(on));
	gattlib_register_notification(s->c, notification_cb, s);
	if (gattlib_notification_start(s->c, &s->uuid)) {
		dprintf(1,"error: failed to start bluetooth notification.\n");
		gattlib_disconnect(s->c);
		return 1;
	}
	dprintf(1,"s->c: %p\n", s->c);

	return 0;
}

static int bt_read(void *handle,...) {
	bt_session_t *s = handle;
	uint8_t *buf;
	int buflen, len, retries;
	va_list ap;

	if (!s->c) return -1;

	va_start(ap,handle);
	buf = va_arg(ap, uint8_t *);
	buflen = va_arg(ap, int);
	va_end(ap);

	dprintf(1,"buf: %p, buflen: %d\n", buf, buflen);

	retries=3;
	while(1) {
		dprintf(1,"s->len: %d\n", s->len);
		if (!s->len) {
			if (!--retries) return 0;
			sleep(1);
			continue;
		}
		len = (s->len > buflen ? buflen : s->len);
		dprintf(1,"len: %d\n", len);
		memcpy(buf,s->data,len);
		s->len = 0;
		break;
	}

	return len;
}

static int bt_write(void *handle,...) {
	bt_session_t *s = handle;
	uint8_t *buf;
	int buflen;
	va_list ap;

	if (!s->c) return -1;

	va_start(ap,handle);
	buf = va_arg(ap, uint8_t *);
	buflen = va_arg(ap, int);
	va_end(ap);

	dprintf(1,"buf: %p, buflen: %d\n", buf, buflen);

	s->len = 0;
	s->cbcnt = 0;
	dprintf(1,"s->c: %p\n", s->c);
	if (gattlib_write_char_by_uuid(s->c, &s->uuid, buf, buflen)) return -1;

	return buflen;
}


static int bt_close(void *handle) {
	bt_session_t *s = handle;

	dprintf(1,"s->c: %p\n",s->c);
	if (s->c) {
		gattlib_notification_stop(s->c, &s->uuid);
		gattlib_disconnect(s->c);
		s->c = 0;
	}
	return 0;
}

mybmm_module_t bt_module = {
	MYBMM_MODTYPE_TRANSPORT,
	"bt",
	0,
	bt_init,
	bt_new,
	bt_open,
	bt_read,
	bt_write,
	bt_close
};
