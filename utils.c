
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "mybmm.h"
#include "utils.h"

void _bindump(long offset,void *bptr,int len) {
	char line[128], *buf = bptr;
	int end;
	register char *ptr;
	register int x,y;

//	printf("buf: %p, len: %d\n", buf, len);
#ifdef __WIN32__
	if (buf == (void *)0xBAADF00D) return;
#endif

	for(x=y=0; x < len; x += 16) {
		sprintf(line,"%04lX: ",offset);
		ptr = line + strlen(line);
		end=(x+16 >= len ? len : x+16);
		for(y=x; y < end; y++) {
			sprintf(ptr,"%02X ",buf[y]);
			ptr += 3;
		}
		for(y=end; y < x+17; y++) {
			sprintf(ptr,"   ");
			ptr += 3;
		}
		for(y=x; y < end; y++) {
			if (buf[y] > 31 && buf[y] < 127)
				*ptr++ = buf[y];
			else
				*ptr++ = '.';
		}
		for(y=end; y < x+16; y++) *ptr++ = ' ';
		*ptr = 0;
		printf("%s\n",line);
		offset += 16;
	}
}

void bindump(char *label,void *bptr,int len) {
	printf("%s(%d):\n",label,len);
	_bindump(0,bptr,len);
}

char *trim(char *string) {
	register char *src,*dest;

	/* If string is empty, just return it */
	if (*string == '\0') return string;

	/* Trim the front */
	src = string;
//	while(isspace((int)*src) && *src != '\t') src++;
	while(isspace((int)*src) || (*src > 0 && *src < 32)) src++;
	dest = string;
	while(*src != '\0') *dest++ = *src++;

	/* Trim the back */
	*dest-- = '\0';
	while((dest >= string) && isspace((int)*dest)) dest--;
	*(dest+1) = '\0';

	return string;
}

char *strele(int num,char *delimiter,char *string) {
	static char return_info[1024];
	register char *src,*dptr,*eptr,*cptr;
	register char *dest, qc;
	register int count;

#ifdef DEBUG_STRELE
	printf("Element: %d, delimiter: %s, string: %s\n",num,delimiter,string);
#endif

	eptr = string;
	dptr = delimiter;
	dest = return_info;
	count = 0;
	qc = 0;
	for(src = string; *src != '\0'; src++) {
#ifdef DEBUG_STRELE
		printf("src: %d, qc: %d\n", *src, qc);
#endif
		if (qc) {
			if (*src == qc) qc = 0;
			continue;
		} else {
			if (*src == '\"' || *src == '\'')  {
				qc = *src;
				cptr++;
			}
		}
		if (isspace(*src)) *src = 32;
#ifdef DEBUG_STRELE
		if (*src)
			printf("src: %c == ",*src);
		else
			printf("src: (null) == ");
		if (*dptr != 32)
			printf("dptr: %c\n",*dptr);
		else if (*dptr == 32)
			printf("dptr: (space)\n");
		else
			printf("dptr: (null)\n");
#endif
		if (*src == *dptr) {
			cptr = src+1;
			dptr++;
#ifdef DEBUG_STRELE
			if (*cptr != '\0')
				printf("cptr: %c == ",*cptr);
			else
				printf("cptr: (null) == ");
			if (*dptr != '\0')
				printf("dptr: %c\n",*dptr);
			else
				printf("dptr: (null)\n");
#endif
			while(*cptr == *dptr && *cptr != '\0' && *dptr != '\0') {
				cptr++;
				dptr++;
#ifdef DEBUG_STRELE
				if (*cptr != '\0')
					printf("cptr: %c == ",*cptr);
				else
					printf("cptr: (null) == ");
				if (*dptr != '\0')
					printf("dptr: %c\n",*dptr);
				else
					printf("dptr: (null)\n");
#endif
				if (*dptr == '\0' || *cptr == '\0') {
#ifdef DEBUG_STRELE
					printf("Breaking...\n");
#endif
					break;
				}
/*
				dptr++;
				if (*dptr == '\0') break;
				cptr++;
				if (*cptr == '\0') break;
*/
			}
#ifdef DEBUG_STRELE
			if (*cptr != '\0')
				printf("cptr: %c == ",*cptr);
			else
				printf("cptr: (null) == ");
			if (*dptr != '\0')
				printf("dptr: %c\n",*dptr);
			else
				printf("dptr: (null)\n");
#endif
			if (*dptr == '\0') {
#ifdef DEBUG_STRELE
				printf("Count: %d, num: %d\n",count,num);
#endif
				if (count == num) break;
				if (cptr > src+1) src = cptr-1;
				eptr = src+1;
				count++;
//				printf("eptr[0]: %c\n", eptr[0]);
				if (*eptr == '\"' || *eptr == '\'') eptr++;
#ifdef DEBUG_STRELE
				printf("eptr: %s, src: %s\n",eptr,src+1);
#endif
			}
			dptr = delimiter;
		}
	}
#ifdef DEBUG_STRELE
	printf("Count: %d, num: %d\n",count,num);
#endif
	if (count == num) {
#ifdef DEBUG_STRELE
		printf("eptr: %s\n",eptr);
		printf("src: %s\n",src);
#endif
		while(eptr < src) {
			if (*eptr == '\"' || *eptr == '\'') break;
			*dest++ = *eptr++;
		}
	}
	*dest = '\0';
#ifdef DEBUG_STRELE
	printf("Returning: %s\n",return_info);
#endif
	return(return_info);
}

int is_ip(char *string) {
	register char *ptr;
	int dots,digits;

	dprintf(7,"string: %s\n", string);

	digits = dots = 0;
	for(ptr=string; *ptr; ptr++) {
		if (*ptr == '.') {
			if (!digits) goto is_ip_error;
			if (++dots > 4) goto is_ip_error;
			digits = 0;
		} else if (isdigit((int)*ptr)) {
			if (++digits > 3) goto is_ip_error;
		}
	}
	dprintf(7,"dots: %d\n", dots);

	return (dots == 4 ? 1 : 0);
is_ip_error:
	return 0;
}

