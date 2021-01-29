
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
/*
 * Unified UUID/GUID definition
 *
 * Copyright (C) 2009, 2016 Intel Corp.
 *	Huang Ying <ying.huang@intel.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <ctype.h>
#define UUID_STRING_LEN 37
struct uuid_s {
	unsigned char b[37];
};
typedef struct uuid_s uuid_t;
typedef unsigned char u8;
typedef unsigned char __u8;
typedef unsigned char bool;
#define false 0
#define true 1
//const guid_t guid_null;
const uuid_t uuid_null;
//const u8 guid_index[16] = {3,2,1,0,5,4,7,6,8,9,10,11,12,13,14,15};
const u8 uuid_index[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
int hex_to_bin(char ch) {
	if ((ch >= '0') && (ch <= '9'))
		return ch - '0';
	ch = tolower(ch);
	if ((ch >= 'a') && (ch <= 'f'))
		return ch - 'a' + 10;
	return -1;
}
void prandom_bytes(void *buf, size_t bytes)
{
	FILE *fp;

	fp = fopen("/dev/urandom","rb");
	if (fp) fread(buf,1,bytes,fp);
	fclose(fp);
}
void get_random_bytes(void *buf, size_t bytes) {
	prandom_bytes(buf,bytes);
}
/**
 * generate_random_uuid - generate a random UUID
 * @uuid: where to put the generated UUID
 *
 * Random UUID interface
 *
 * Used to create a Boot ID or a filesystem UUID/GUID, but can be
 * useful for other kernel drivers.
 */
void uuid_generate_random(unsigned char uuid[16])
{
	get_random_bytes(uuid, 16);
	/* Set UUID version to 4 --- truly random generation */
	uuid[6] = (uuid[6] & 0x0F) | 0x40;
	/* Set the UUID variant to DCE */
	uuid[8] = (uuid[8] & 0x3F) | 0x80;
}
static void __uuid_gen_common(__u8 b[16])
{
	prandom_bytes(b, 16);
	/* reversion 0b10 */
	b[8] = (b[8] & 0x3F) | 0x80;
}
#if 0
void guid_gen(guid_t *lu)
{
	__uuid_gen_common(lu->b);
	/* version 4 : random generation */
	lu->b[7] = (lu->b[7] & 0x0F) | 0x40;
}
#endif
//EXPORT_SYMBOL_GPL(guid_gen);
void uuid_gen(uuid_t *bu)
{
	__uuid_gen_common(bu->b);
	/* version 4 : random generation */
	bu->b[6] = (bu->b[6] & 0x0F) | 0x40;
}
/**
 * uuid_is_valid - checks if a UUID string is valid
 * @uuid:	UUID string to check
 *
 * Description:
 * It checks if the UUID string is following the format:
 *	xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
 *
 * where x is a hex digit.
 *
 * Return: true if input is valid UUID string.
 */
bool uuid_is_valid(const char *uuid)
{
	unsigned int i;
	for (i = 0; i < UUID_STRING_LEN; i++) {
		if (i == 8 || i == 13 || i == 18 || i == 23) {
			if (uuid[i] != '-')
				return false;
		} else if (!isxdigit(uuid[i])) {
			return false;
		}
	}
	return true;
}
//EXPORT_SYMBOL(uuid_is_valid);
static int __uuid_parse(const char *uuid, __u8 b[16], const u8 ei[16])
{
	static const u8 si[16] = {0,2,4,6,9,11,14,16,19,21,24,26,28,30,32,34};
	unsigned int i;
	if (!uuid_is_valid(uuid))
		return -EINVAL;
	for (i = 0; i < 16; i++) {
		int hi = hex_to_bin(uuid[si[i] + 0]);
		int lo = hex_to_bin(uuid[si[i] + 1]);
		b[ei[i]] = (hi << 4) | lo;
	}
	return 0;
}
#if 0
int guid_parse(const char *uuid, guid_t *u)
{
	return __uuid_parse(uuid, u->b, guid_index);
}
#endif
int uuid_parse(const char *uuid, uuid_t *u)
{
	return __uuid_parse(uuid, u->b, uuid_index);
}
/*
 * unparse.c -- convert a UUID to string
 *
 * Copyright (C) 1996, 1997 Theodore Ts'o.
 *
 * %Begin-Header%
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, and the entire permission notice in its entirety,
 *    including the disclaimer of warranties.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, ALL OF
 * WHICH ARE HEREBY DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF NOT ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * %End-Header%
 */

/*
 * Offset between 15-Oct-1582 and 1-Jan-70
 */
#define TIME_OFFSET_HIGH 0x01B21DD2
#define TIME_OFFSET_LOW  0x13814000

struct uuid {
	uint32_t	time_low;
	uint16_t	time_mid;
	uint16_t	time_hi_and_version;
	uint16_t	clock_seq;
	uint8_t	node[6];
};

static const char *fmt_lower =
	"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x";

static const char *fmt_upper =
	"%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X";

#ifdef UUID_UNPARSE_DEFAULT_UPPER
#define FMT_DEFAULT fmt_upper
#else
#define FMT_DEFAULT fmt_lower
#endif

void uuid_unpack(const uuid_t *in, struct uuid *uu)
{
	const uint8_t	*ptr = (uint8_t *) in;
	uint32_t		tmp;

	tmp = *ptr++;
	tmp = (tmp << 8) | *ptr++;
	tmp = (tmp << 8) | *ptr++;
	tmp = (tmp << 8) | *ptr++;
	uu->time_low = tmp;

	tmp = *ptr++;
	tmp = (tmp << 8) | *ptr++;
	uu->time_mid = tmp;

	tmp = *ptr++;
	tmp = (tmp << 8) | *ptr++;
	uu->time_hi_and_version = tmp;

	tmp = *ptr++;
	tmp = (tmp << 8) | *ptr++;
	uu->clock_seq = tmp;

	memcpy(uu->node, ptr, 6);
}

static void uuid_unparse_x(const uuid_t *uu, char *out, const char *fmt) {
	struct uuid uuid;

	uuid_unpack(uu, &uuid);
	sprintf(out, fmt,
		uuid.time_low, uuid.time_mid, uuid.time_hi_and_version,
		uuid.clock_seq >> 8, uuid.clock_seq & 0xFF,
		uuid.node[0], uuid.node[1], uuid.node[2],
		uuid.node[3], uuid.node[4], uuid.node[5]);
}

void uuid_unparse_lower(const uuid_t *uu, char *out)
{
	uuid_unparse_x(uu, out,	fmt_lower);
}

void uuid_unparse_upper(const uuid_t *uu, char *out)
{
	uuid_unparse_x(uu, out,	fmt_upper);
}

void my_uuid_unparse(const uuid_t *uu, char *out)
{
	uuid_unparse_x(uu, out, FMT_DEFAULT);
}
