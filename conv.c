
#ifdef DEBUG
#undef DEBUG
#endif
//#define DEBUG 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "conv.h"
#include "list.h"

#if DEBUG
#define dprintf(format, args...) printf("%s(%d): " format "\n",__FUNCTION__,__LINE__, ## args)
#else
#define dprintf(format, args...) /* noop */
#endif

static char conv_temp[32767];
extern char *trim(char *);

/*
 * DATA_TYPE_BYTE conversion functions
*/

void byte2chr(char *dest,int dlen,byte *src,int slen) {
	register int x;

	sprintf(dest,"%d",*src);
	x = strlen(dest);
	while(x < dlen) dest[x++] = ' ';
	return;
}

void byte2str(char *dest,int dlen,byte *src,int slen) {
	sprintf(dest,"%d",*src);
	return;
}

void byte2byte(byte *dest,int dlen,byte *src,int slen) {
	*dest = *src;
	return;
}

void byte2short(short *dest,int dlen,byte *src,int slen) {
	*dest = *src;
	return;
}

void byte2int(int *dest,int dlen,byte *src,int slen) {
	*dest = *src;
	return;
}

void byte2long(long *dest,int dlen,byte *src,int slen) {
	*dest = *src;
	return;
}

void byte2quad(myquad_t *dest,int dlen,byte *src,int slen) {
	*dest = *src;
	return;
}

void byte2float(float *dest,int dlen,byte *src,int slen) {
	*dest = *src;
	return;
}

void byte2dbl(double *dest,int dlen,byte *src,int slen) {
	*dest = *src;
	return;
}

void byte2log(int *dest,int dlen,byte *src,int slen) {
	*dest = (*src == 0 ? 0 : 1);
	return;
}

void byte2date(char *dest,int dlen,byte *src,int slen) {
	*dest = 0;
	return;
}

void byte2list(list *dest,int dlen,byte *src,int slen) {
	if (!*dest) *dest = list_create();
	return;
}


/*
 * DATA_TYPE_CHAR conversion functions
*/

void chr2chr(char *dest,int dlen,char *src,int slen) {
	register char *dptr = dest;
	char *dend = dest + dlen;
	register char *sptr = src;
	char *send = src + slen;

	/* Copy the fields */
	dprintf("chr2chr: dptr: %p, dlen: %d, dend: %p",
		dptr,dlen,dend);
	dprintf("chr2chr: sptr: %p, slen: %d, send: %p",
		sptr,slen,send);
	while(sptr < send && dptr < dend) {
		dprintf("sptr: %p, *sptr: %c",sptr,*sptr);
		*dptr++ = *sptr++;
		dprintf("dptr: %p, *dend: %p",dptr,dend);
	}

	/* Blank fill dest */
	dprintf("dptr: %p, dend: %p",dptr,dend);
	while(dptr < dend) *dptr++ = ' ';
	dprintf("dptr: %p, dend: %p",dptr,dend);
	dprintf("chr2chr: done.");
	return;
}

void chr2str(char *dest,int dlen,char *src,int slen) {
	register char *dptr = dest;
	char *dend = dest + dlen;
	register char *sptr = src;
	char *send = src + slen;

	/* Copy the fields */
	dprintf("chr2str: dptr: %p, dlen: %d, dend: %p",
		dptr,dlen,dend);
	dprintf("chr2str: sptr: %p, slen: %d, send: %p",
		sptr,slen,send);
	while(sptr < send && dptr < dend) {
		dprintf("sptr: %p, *sptr: %c",sptr,*sptr);
		*dptr++ = *sptr++;
		dprintf("dptr: %p, *dend: %p",dptr,dend);
	}

	/* Trim & zero-terminate dest */
	dprintf("dptr: %p, dend: %p",dptr,dend);
	while(isspace((int)*dptr)) dptr--;
	*dptr = 0;
	dprintf("dptr: %p, dend: %p",dptr,dend);
	return;
}

void chr2byte(byte *dest,int dlen,char *src,int slen) {
	int len = (slen >= sizeof(conv_temp) ? sizeof(conv_temp)-1 : slen);

	bcopy(src,conv_temp,len);
	conv_temp[len] = 0;
	dprintf("chr2byte: conv_temp: %s",conv_temp);
	*dest = atoi(conv_temp);
	return;
}

void chr2short(short *dest,int dlen,char *src,int slen) {
	int len = (slen >= sizeof(conv_temp) ? sizeof(conv_temp)-1 : slen);

	bcopy(src,conv_temp,len);
	conv_temp[len] = 0;
	dprintf("chr2short: conv_temp: %s",conv_temp);
	*dest = atoi(conv_temp);
	return;
}

void chr2int(int *dest,int dlen,char *src,int slen) {
	int len = (slen >= sizeof(conv_temp) ? sizeof(conv_temp)-1 : slen);

	bcopy(src,conv_temp,len);
	conv_temp[len] = 0;
	dprintf("chr2int: conv_temp: %s",conv_temp);
	*dest = atoi(conv_temp);
	return;
}

void chr2long(long *dest,int dlen,char *src,int slen) {
	int len = (slen >= sizeof(conv_temp) ? sizeof(conv_temp)-1 : slen);

	bcopy(src,conv_temp,len);
	conv_temp[len] = 0;
	dprintf("chr2long: conv_temp: %s",conv_temp);
	*dest = atol(conv_temp);
	return;
}

void chr2quad(myquad_t *dest,int dlen,char *src,int slen) {
	int len = (slen >= sizeof(conv_temp) ? sizeof(conv_temp)-1 : slen);

#if !defined(__CYGWIN__)
	bcopy(src,conv_temp,len);
	conv_temp[len] = 0;
	dprintf("chr2quad: conv_temp: %s",conv_temp);
	*dest = strtoll(conv_temp,0,0);
#else
	*dest = 0;
#endif
	return;
}

void chr2float(float *dest,int dlen,char *src,int slen) {
	int len = (slen >= sizeof(conv_temp) ? sizeof(conv_temp)-1 : slen);

	bcopy(src,conv_temp,len);
	conv_temp[len] = 0;
	dprintf("chr2float: conv_temp: %s",conv_temp);
	*dest = (float) atof(conv_temp);
	return;
}

void chr2dbl(double *dest,int dlen,char *src,int slen) {
	int len = (slen >= sizeof(conv_temp) ? sizeof(conv_temp)-1 : slen);

	bcopy(src,conv_temp,len);
	conv_temp[len] = 0;
	dprintf("chr2dbl: conv_temp: %s",conv_temp);
	*dest = atof(conv_temp);
	return;
}

void chr2log(int *dest,int dlen,char *src,int slen) {
	int len = (slen >= sizeof(conv_temp) ? sizeof(conv_temp)-1 : slen);

	bcopy(src,conv_temp,len);
	conv_temp[len] = 0;
	dprintf("chr2log: conv_temp: %s",conv_temp);
	*dest = (atoi(conv_temp) == 0 ? 0 : 1);
	return;
}

void chr2list(list *dest,int dlen,char *src,int slen) {
	printf("chr2list: dest: %p, dlen: %d, src: %s, slen: %d\n", dest, dlen, src, slen);
	if (!*dest) *dest = list_create();
	return;
}

#define FMT "%f"

/*
 * DATA_TYPE_DOUBLE conversion functions
*/

void dbl2chr(char *dest,int dlen,double *src,int slen) {
	register int x;

	snprintf(dest,dlen,FMT,*src);
	x = strlen(dest);
	while(x < dlen) dest[x++] = ' ';
	return;
}

void dbl2str(char *dest,int dlen,double *src,int slen) {
	dprintf("dbl2str: src: %lf",*src);
	snprintf(dest,dlen-1,FMT,*src);
	dprintf("dbl2str: dest: %s",dest);
	return;
}

void dbl2byte(byte *dest,int dlen,double *src,int slen) {
	*dest = *src;
	return;
}

void dbl2short(short *dest,int dlen,double *src,int slen) {
	*dest = *src;
	return;
}

void dbl2int(int *dest,int dlen,double *src,int slen) {
	*dest = *src;
	return;
}

void dbl2long(long *dest,int dlen,double *src,int slen) {
	*dest = *src;
	return;
}

void dbl2quad(myquad_t *dest,int dlen,double *src,int slen) {
	*dest = *src;
	return;
}

void dbl2float(float *dest,int dlen,double *src,int slen) {
	*dest = *src;
	return;
}

void dbl2dbl(double *dest,int dlen,double *src,int slen) {
	*dest = *src;
	return;
}

void dbl2log(int *dest,int dlen,double *src,int slen) {
	*dest = (*src == 0.0 ? 0 : 1);
	return;
}

void dbl2date(char *dest,int dlen,double *src,int slen) {
	*dest = 0;
	return;
}

void dbl2list(list *dest,int dlen,double *src,int slen) {
	if (!*dest) *dest = list_create();
	return;
}

/*
 * DATA_TYPE_FLOAT conversion functions
*/

void float2chr(char *dest,int dlen,float *src,int slen) {
	register int x;

	sprintf(dest,"%f",*src);
	x = strlen(dest);
	while(x < dlen) dest[x++] = ' ';
	return;
}

void float2str(char *dest,int dlen,float *src,int slen) {
	sprintf(dest,"%f",*src);
	return;
}

void float2byte(byte *dest,int dlen,float *src,int slen) {
	*dest = *src;
	return;
}

void float2short(short *dest,int dlen,float *src,int slen) {
	*dest = *src;
	return;
}

void float2int(int *dest,int dlen,float *src,int slen) {
	*dest = *src;
	return;
}

void float2long(long *dest,int dlen,float *src,int slen) {
	*dest = *src;
	return;
}

void float2quad(myquad_t *dest,int dlen,float *src,int slen) {
	*dest = *src;
	return;
}

void float2float(float *dest,int dlen,float *src,int slen) {
	*dest = *src;
	return;
}

void float2dbl(double *dest,int dlen,float *src,int slen) {
	*dest = *src;
	return;
}

void float2log(int *dest,int dlen,float *src,int slen) {
	*dest = (*src == 0.0 ? 0 : 1);
	return;
}

void float2date(char *dest,int dlen,float *src,int slen) {
	*dest = 0;
	return;
}

void float2list(list *dest,int dlen,float *src,int slen) {
	if (!*dest) *dest = list_create();
	return;
}

/*
 * DATA_TYPE_INT conversion functions
*/

void int2chr(char *dest,int dlen,int *src,int slen) {
	register int x;

	sprintf(dest,"%d",*src);
	x = strlen(dest);
	while(x < dlen) dest[x++] = ' ';
	return;
}

void int2str(char *dest,int dlen,int *src,int slen) {
	sprintf(dest,"%d",*src);
	return;
}

void int2byte(byte *dest,int dlen,int *src,int slen) {
	*dest = *src;
	return;
}

void int2short(short *dest,int dlen,int *src,int slen) {
	*dest = *src;
	return;
}

void int2int(int *dest,int dlen,int *src,int slen) {
	*dest = *src;
	return;
}

void int2long(long *dest,int dlen,int *src,int slen) {
	*dest = *src;
	return;
}

void int2quad(myquad_t *dest,int dlen,int *src,int slen) {
	*dest = *src;
	return;
}

void int2float(float *dest,int dlen,int *src,int slen) {
	*dest = *src;
	return;
}

void int2dbl(double *dest,int dlen,int *src,int slen) {
	*dest = *src;
	return;
}

void int2log(int *dest,int dlen,int *src,int slen) {
	*dest = (*src == 0 ? 0 : 1);
	return;
}

void int2date(char *dest,int dlen,int *src,int slen) {
	*dest = 0;
	return;
}

void int2list(list *dest,int dlen,int *src,int slen) {
	register int x,len = slen;

	*dest = list_create();
	if (!*dest) return;
	len = (slen ? slen : 1);
	for(x=0; x < len; x++) list_add(*dest,(void *)&src[x],sizeof(int));
	return;
}

void list2str(char *dest,int dlen,list *src,int slen) {
	int first,dl,sl;
	char *ptr;

	dprintf("dest: %p, dlen: %d, list: %p, slen: %d", dest, dlen, src, slen);
	dl = *dest = 0;
	if (!src) return;
	first = 1;
	list_reset(*src);
	while( (ptr = list_get_next(*src)) != 0) {
		sl = strlen(ptr);
		dprintf("list2str: ptr: %s, len: %d",ptr,sl);
		dprintf("dl: %d, sl: %d, dlen: %d",dl,sl,dlen);
		if (dl + sl >= dlen) break;
		if (first == 0) strcat(dest,",");
		else first = 0;
		strcat(dest,ptr);
	}
	dprintf("list2str: dest: %s",dest);
	return;
}

void list2chr(char *dest,int dlen,list *src,int slen) {
	register int x;

	list2str(dest,dlen,src,slen);
	x = strlen(dest);
	while(x < dlen) dest[x++] = ' ';
	return;
}

void list2byte(byte *dest,int dlen,list *src,int slen) {
	*dest = 0;
}

void list2short(short *dest,int dlen,list *src,int slen) {
	*dest = 0;
}

void list2int(int *dest,int dlen,list *src,int slen) {
	*dest = 0;
}

void list2long(long *dest,int dlen,list *src,int slen) {
	*dest = 0;
}

void list2quad(myquad_t *dest,int dlen,list *src,int slen) {
	*dest = 0;
}

void list2float(float *dest,int dlen,list *src,int slen) {
	*dest = 0;
}

void list2dbl(double *dest,int dlen,list *src,int slen) {
	*dest = 0;
}

void list2log(int *dest,int dlen,list *src,int slen) {
	*dest = 0;
}

void list2date(char *dest,int dlen,list *src,int slen) {
	*dest = 0;
}

void list2list(list *dest,int dlen,list *src,int slen) {
	if (!*dest) *dest = list_create();
	list_add_list(*dest,*src);
	return;
}

void log2chr(char *dest,int dlen,int *src,int slen) {
	register int x;

	sprintf(dest,"%s",(*src == 0 ? "False" : "True"));
	x = strlen(dest);
	while(x < dlen) dest[x++] = ' ';
	return;
}

void log2str(char *dest,int dlen,int *src,int slen) {
	sprintf(dest,"%s",(*src == 0 ? "False" : "True"));
	return;
}

void log2list(list *dest,int dlen,int *src,int slen) {
	if (!*dest) *dest = list_create();
	return;
}

/*
 * DATA_TYPE_LONG conversion functions
*/

void long2chr(char *dest,int dlen,long *src,int slen) {
	register int x;

	sprintf(dest,"%ld",*src);
	x = strlen(dest);
	while(x < dlen) dest[x++] = ' ';
	return;
}

void long2str(char *dest,int dlen,long *src,int slen) {
	sprintf(dest,"%ld",*src);
	return;
}

void long2byte(byte *dest,int dlen,long *src,int slen) {
	*dest = *src;
	return;
}

void long2short(short *dest,int dlen,long *src,int slen) {
	*dest = *src;
	return;
}

void long2int(int *dest,int dlen,long *src,int slen) {
	*dest = *src;
	return;
}

void long2long(long *dest,int dlen,long *src,int slen) {
	*dest = *src;
	return;
}

void long2quad(myquad_t *dest,int dlen,long *src,int slen) {
	*dest = *src;
	return;
}

void long2float(float *dest,int dlen,long *src,int slen) {
	*dest = *src;
	return;
}

void long2dbl(double *dest,int dlen,long *src,int slen) {
	*dest = *src;
	return;
}

void long2log(int *dest,int dlen,long *src,int slen) {
	*dest = (*src == 0 ? 0 : 1);
	return;
}

void long2date(char *dest,int dlen,long *src,int slen) {
	*dest = 0;
	return;
}

void long2list(list *dest,int dlen,long *src,int slen) {
	if (!*dest) *dest = list_create();
	return;
}

/*
 * DATA_TYPE_QUAD conversion functions
*/

void quad2chr(char *dest,int dlen,myquad_t *src,int slen) {
	register int x;

	sprintf(dest,"%lld",*src);
	x = strlen(dest);
	while(x < dlen) dest[x++] = ' ';
	return;
}

void quad2str(char *dest,int dlen,myquad_t *src,int slen) {
	snprintf(dest,dlen,"%lld",*src);
	return;
}

void quad2byte(byte *dest,int dlen,myquad_t *src,int slen) {
	*dest = *src;
	return;
}

void quad2short(short *dest,int dlen,myquad_t *src,int slen) {
	*dest = (short) *src;
	return;
}

void quad2int(int *dest,int dlen,myquad_t *src,int slen) {
	*dest = *src;
	return;
}

void quad2long(long *dest,int dlen,myquad_t *src,int slen) {
	*dest = *src;
	return;
}

void quad2quad(myquad_t *dest,int dlen,myquad_t *src,int slen) {
	*dest = *src;
	return;
}

void quad2float(float *dest,int dlen,myquad_t *src,int slen) {
	*dest = (float) *src;
	return;
}

void quad2dbl(double *dest,int dlen,myquad_t *src,int slen) {
	*dest = *src;
	return;
}

void quad2log(int *dest,int dlen,myquad_t *src,int slen) {
	*dest = (*src == 0 ? 0 : 1);
	return;
}

void quad2date(char *dest,int dlen,myquad_t *src,int slen) {
	*dest = 0;
	return;
}

void quad2list(list *dest,int dlen,myquad_t *src,int slen) {
	if (!*dest) *dest = list_create();
	return;
}

/*
 * DATA_TYPE_SHORT conversion functions
*/

void short2chr(char *dest,int dlen,short *src,int slen) {
	register int x;

	sprintf(dest,"%d",*src);
	x = strlen(dest);
	while(x < dlen) dest[x++] = ' ';
	return;
}

void short2str(char *dest,int dlen,short *src,int slen) {
	sprintf(dest,"%d",*src);
	return;
}

void short2byte(byte *dest,int dlen,short *src,int slen) {
	*dest = *src;
	return;
}

void short2short(short *dest,int dlen,short *src,int slen) {
	*dest = *src;
	return;
}

void short2int(int *dest,int dlen,short *src,int slen) {
	*dest = *src;
	return;
}

void short2long(long *dest,int dlen,short *src,int slen) {
	*dest = *src;
	return;
}

void short2quad(myquad_t *dest,int dlen,short *src,int slen) {
	*dest = *src;
	return;
}

void short2float(float *dest,int dlen,short *src,int slen) {
	*dest = *src;
	return;
}

void short2dbl(double *dest,int dlen,short *src,int slen) {
	*dest = *src;
	return;
}

void short2log(int *dest,int dlen,short *src,int slen) {
	*dest = (*src == 0 ? 0 : 1);
	return;
}

void short2date(char *dest,int dlen,short *src,int slen) {
	*dest = 0;
	return;
}

void short2list(list *dest,int dlen,short *src,int slen) {
	if (!*dest) *dest = list_create();
	return;
}

/*
 * DATA_TYPE_STRING conversion functions
*/

void str2chr(char *dest,int dlen,char *src,int slen) {
	register int x,y;

	dprintf("src: %s",src);
	x=y=0;
	while(x < slen && src[x] != 0 && y < dlen) dest[y++] = src[x++];
	while(y < dlen) dest[y++] = ' ';

	return;
}

void str2str(char *dest,int dlen,char *src,int slen) {

	dprintf("src: %s",src);

	/* Copy the fields */
	*dest = 0;
	dprintf("dlen: %d", dlen);
	strncat(dest,src,dlen);
	trim(dest);

	dprintf("dest: %s",dest);
	return;
}

void str2byte(byte *dest,int dlen,char *src,int slen) {
	dprintf("src: %s",src);
	*dest = atoi(src);
	dprintf("src: %s",src);
	return;
}

void str2short(short *dest,int dlen,char *src,int slen) {
	dprintf("src: %s",src);
	*dest = atoi(src);
	dprintf("dest: %d",*dest);
	return;
}

void str2int(int *dest,int dlen,char *src,int slen) {
	dprintf("src: %s",src);
	*dest = atoi(src);
	dprintf("dest: %d",*dest);
	return;
}

void str2long(long *dest,int dlen,char *src,int slen) {
	dprintf("src: %s",src);
	*dest = atol(src);
	dprintf("dest: %ld",*dest);
	return;
}

void str2quad(myquad_t *dest,int dlen,char *src,int slen) {
	dprintf("src: %s",src);
#ifndef __CYGWIN__
	*dest = strtoll(src, 0, 0);
#else
	*dest = 0;
#endif
	dprintf("dest: %lld",*dest);
	return;
}

void str2float(float *dest,int dlen,char *src,int slen) {
	dprintf("src: %s",src);
	*dest = (float) atof(src);
	dprintf("dest: %f",*dest);
	return;
}

void str2dbl(double *dest,int dlen,char *src,int slen) {
	*dest = atof(src);
	return;
}

void str2log(int *dest,int dlen,char *src,int slen) {
	register char *ptr, ch;

	dprintf("src: %s",src);
	for(ptr = src; *ptr && isspace(*ptr); ptr++);
	dprintf("ptr: %s",ptr);
	ch = toupper(*ptr);
	dprintf("ch: %c",ch);
	*dest = (ch == 'T' || ch == 'Y' || ch == '1' ? 1 : 0);
	dprintf("dest: %d",*dest);
	return;
}

void str2date(char *dest,int dlen,char *src,int slen) {
	int len = (slen > dlen ? dlen : slen);

	*dest = 0;
	strncat(dest,src,len);
	return;
}

void str2list(list *dest,int dlen,char *src,int slen) {
	register char *ptr;
	register int x;

	dprintf("dest: %p, dlen: %d, src: %s, slen: %d", dest, dlen, src, slen);
	if (!*dest) *dest = list_create();
	dprintf("src: %s",src);
	x = 0;
	for(ptr = src; *ptr; ptr++) {
		if (*ptr == ',') {
			x = conv_temp[x] = 0;
			trim(conv_temp);
			list_add(*dest,conv_temp,strlen(conv_temp)+1);
		} else
			conv_temp[x++] = *ptr;
	}
	dprintf("x: %d", x);
	if (x) {
		conv_temp[x] = 0;
		dprintf("%s", conv_temp);
		trim(conv_temp);
		list_add(*dest,conv_temp,strlen(conv_temp)+1);
	}
#ifdef _DEBUG
	dprintf("list:");
	list_reset(*dest);
	while( (ptr = list_get_next(*dest)) != 0)
		dprintf("\t%s",ptr);
	list_reset(*dest);
#endif
	return;
}

#define chr2date chr2str
#define date2chr str2chr
#define date2str str2str
#define date2byte str2byte
#define date2short str2short
#define date2int str2int
#define date2long str2long
#define date2quad str2quad
#define date2float str2float
#define date2dbl str2dbl
#define date2log str2log
#define date2date str2str
#define date2list str2list
#define log2byte int2byte
#define log2short int2short
#define log2int int2int
#define log2long int2long
#define log2quad int2quad
#define log2float int2float
#define log2dbl int2dbl
#define log2log int2int
#define log2date int2date


#ifdef DEBUG
static char *conv_func_names[13][13] = {
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{	0,
		"chr2chr",
		"chr2str",
		"chr2byte",
		"chr2short",
		"chr2int",
		"chr2long",
		"chr2quad",
		"chr2float",
		"chr2dbl",
		"chr2log",
		"chr2date",
		"chr2list",
	},
	{	0,
		"str2chr",
		"str2str",
		"str2byte",
		"str2short",
		"str2int",
		"str2long",
		"str2quad",
		"str2float",
		"str2dbl",
		"str2log",
		"str2date",
		"str2list",
	},
	{	0,
		"byte2chr",
		"byte2str",
		"byte2byte",
		"byte2short",
		"byte2int",
		"byte2long",
		"byte2quad",
		"byte2float",
		"byte2dbl",
		"byte2log",
		"byte2date",
		"byte2list",
	},
	{	0,
		"short2chr",
		"short2str",
		"short2byte",
		"short2short",
		"short2int",
		"short2long",
		"short2quad",
		"short2float",
		"short2dbl",
		"short2log",
		"short2date",
		"short2list",
	},
	{	0,
		"int2chr",
		"int2str",
		"int2byte",
		"int2short",
		"int2int",
		"int2long",
		"int2quad",
		"int2float",
		"int2dbl",
		"int2log",
		"int2date",
		"int2list",
	},
	{	0,
		"long2chr",
		"long2str",
		"long2byte",
		"long2short",
		"long2int",
		"long2long",
		"long2quad",
		"long2float",
		"long2dbl",
		"long2log",
		"long2date",
		"long2list",
	},
	{	0,
		"quad2chr",
		"quad2str",
		"quad2byte",
		"quad2short",
		"quad2int",
		"quad2long",
		"quad2quad",
		"quad2float",
		"quad2dbl",
		"quad2log",
		"quad2date",
		"quad2list",
	},
	{	0,
		"float2chr",
		"float2str",
		"float2byte",
		"float2short",
		"float2int",
		"float2long",
		"float2quad",
		"float2float",
		"float2dbl",
		"float2log",
		"float2date",
		"float2list",
	},
	{	0,
		"dbl2chr",
		"dbl2str",
		"dbl2byte",
		"dbl2short",
		"dbl2int",
		"dbl2long",
		"dbl2quad",
		"dbl2float",
		"dbl2dbl",
		"dbl2log",
		"dbl2date",
		"dbl2list",
	},
	{	0,
		"log2chr",
		"log2str",
		"log2byte",
		"log2short",
		"log2int",
		"log2long",
		"log2quad",
		"log2float",
		"log2dbl",
		"log2log",
		"log2date",
		"log2list",
	},
	{	0,
		"date2chr",
		"date2str",
		"date2byte",
		"date2short",
		"date2int",
		"date2long",
		"date2quad",
		"date2float",
		"date2dbl",
		"date2log",
		"date2date",
		"date2list",
	},
	{	0,
		"list2chr",
		"list2str",
		"list2byte",
		"list2short",
		"list2int",
		"list2long",
		"list2quad",
		"list2float",
		"list2dbl",
		"list2log",
		"list2date",
		"list2list",
	},
};
#endif

typedef void conv_func_t(char *,int,char *,int);
static conv_func_t *conv_funcs[13][13] = {
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{	0,
		(conv_func_t *)chr2chr,
		(conv_func_t *)chr2str,
		(conv_func_t *)chr2byte,
		(conv_func_t *)chr2short,
		(conv_func_t *)chr2int,
		(conv_func_t *)chr2long,
		(conv_func_t *)chr2quad,
		(conv_func_t *)chr2float,
		(conv_func_t *)chr2dbl,
		(conv_func_t *)chr2log,
		(conv_func_t *)chr2date,
		(conv_func_t *)chr2list,
	},
	{	0,
		(conv_func_t *)str2chr,
		(conv_func_t *)str2str,
		(conv_func_t *)str2byte,
		(conv_func_t *)str2short,
		(conv_func_t *)str2int,
		(conv_func_t *)str2long,
		(conv_func_t *)str2quad,
		(conv_func_t *)str2float,
		(conv_func_t *)str2dbl,
		(conv_func_t *)str2log,
		(conv_func_t *)str2date,
		(conv_func_t *)str2list,
	},
	{	0,
		(conv_func_t *)byte2chr,
		(conv_func_t *)byte2str,
		(conv_func_t *)byte2byte,
		(conv_func_t *)byte2short,
		(conv_func_t *)byte2int,
		(conv_func_t *)byte2long,
		(conv_func_t *)byte2quad,
		(conv_func_t *)byte2float,
		(conv_func_t *)byte2dbl,
		(conv_func_t *)byte2log,
		(conv_func_t *)byte2date,
		(conv_func_t *)byte2list,
	},
	{	0,
		(conv_func_t *)short2chr,
		(conv_func_t *)short2str,
		(conv_func_t *)short2byte,
		(conv_func_t *)short2short,
		(conv_func_t *)short2int,
		(conv_func_t *)short2long,
		(conv_func_t *)short2quad,
		(conv_func_t *)short2float,
		(conv_func_t *)short2dbl,
		(conv_func_t *)short2log,
		(conv_func_t *)short2date,
		(conv_func_t *)short2list,
	},
	{	0,
		(conv_func_t *)int2chr,
		(conv_func_t *)int2str,
		(conv_func_t *)int2byte,
		(conv_func_t *)int2short,
		(conv_func_t *)int2int,
		(conv_func_t *)int2long,
		(conv_func_t *)int2quad,
		(conv_func_t *)int2float,
		(conv_func_t *)int2dbl,
		(conv_func_t *)int2log,
		(conv_func_t *)int2date,
		(conv_func_t *)int2list,
	},
	{	0,
		(conv_func_t *)long2chr,
		(conv_func_t *)long2str,
		(conv_func_t *)long2byte,
		(conv_func_t *)long2short,
		(conv_func_t *)long2int,
		(conv_func_t *)long2long,
		(conv_func_t *)long2quad,
		(conv_func_t *)long2float,
		(conv_func_t *)long2dbl,
		(conv_func_t *)long2log,
		(conv_func_t *)long2date,
		(conv_func_t *)long2list,
	},
	{	0,
		(conv_func_t *)quad2chr,
		(conv_func_t *)quad2str,
		(conv_func_t *)quad2byte,
		(conv_func_t *)quad2short,
		(conv_func_t *)quad2int,
		(conv_func_t *)quad2long,
		(conv_func_t *)quad2quad,
		(conv_func_t *)quad2float,
		(conv_func_t *)quad2dbl,
		(conv_func_t *)quad2log,
		(conv_func_t *)quad2date,
		(conv_func_t *)quad2list,
	},
	{	0,
		(conv_func_t *)float2chr,
		(conv_func_t *)float2str,
		(conv_func_t *)float2byte,
		(conv_func_t *)float2short,
		(conv_func_t *)float2int,
		(conv_func_t *)float2long,
		(conv_func_t *)float2quad,
		(conv_func_t *)float2float,
		(conv_func_t *)float2dbl,
		(conv_func_t *)float2log,
		(conv_func_t *)float2date,
		(conv_func_t *)float2list,
	},
	{	0,
		(conv_func_t *)dbl2chr,
		(conv_func_t *)dbl2str,
		(conv_func_t *)dbl2byte,
		(conv_func_t *)dbl2short,
		(conv_func_t *)dbl2int,
		(conv_func_t *)dbl2long,
		(conv_func_t *)dbl2quad,
		(conv_func_t *)dbl2float,
		(conv_func_t *)dbl2dbl,
		(conv_func_t *)dbl2log,
		(conv_func_t *)dbl2date,
		(conv_func_t *)dbl2list,
	},
	{	0,
		(conv_func_t *)log2chr,
		(conv_func_t *)log2str,
		(conv_func_t *)log2byte,
		(conv_func_t *)log2short,
		(conv_func_t *)log2int,
		(conv_func_t *)log2long,
		(conv_func_t *)log2quad,
		(conv_func_t *)log2float,
		(conv_func_t *)log2dbl,
		(conv_func_t *)log2log,
		(conv_func_t *)log2date,
		(conv_func_t *)log2list,
	},
	{	0,
		(conv_func_t *)date2chr,
		(conv_func_t *)date2str,
		(conv_func_t *)date2byte,
		(conv_func_t *)date2short,
		(conv_func_t *)date2int,
		(conv_func_t *)date2long,
		(conv_func_t *)date2quad,
		(conv_func_t *)date2float,
		(conv_func_t *)date2dbl,
		(conv_func_t *)date2log,
		(conv_func_t *)date2date,
		(conv_func_t *)date2list,
	},
	{	0,
		(conv_func_t *)list2chr,
		(conv_func_t *)list2str,
		(conv_func_t *)list2byte,
		(conv_func_t *)list2short,
		(conv_func_t *)list2int,
		(conv_func_t *)list2long,
		(conv_func_t *)list2quad,
		(conv_func_t *)list2float,
		(conv_func_t *)list2dbl,
		(conv_func_t *)list2log,
		(conv_func_t *)list2date,
		(conv_func_t *)list2list,
	},
};

#ifdef DEBUG
static struct {
        int type;
        char *name;
} type_info[] = {
        { DATA_TYPE_UNKNOWN,"UNKNOWN" },
        { DATA_TYPE_CHAR,"CHAR" },
        { DATA_TYPE_STRING,"STRING" },
        { DATA_TYPE_BYTE,"BYTE" },
        { DATA_TYPE_SHORT,"SHORT" },
        { DATA_TYPE_INT,"INT" },
        { DATA_TYPE_LONG,"LONG" },
        { DATA_TYPE_QUAD,"QUAD" },
        { DATA_TYPE_FLOAT,"FLOAT" },
        { DATA_TYPE_DOUBLE,"DOUBLE" },
        { DATA_TYPE_LOGICAL,"BOOLEAN" },
        { DATA_TYPE_DATE,"DATE" },
        { DATA_TYPE_LIST,"LIST" },
        { 0, 0 }
};

char *dt2name(int type) {
        register int x;

        for(x=0; type_info[x].name; x++) {
                if (type_info[x].type == type)
                        return(type_info[x].name);
        }
        return("*BAD TYPE*");
}

int name2dt(char *name) {
        register int x;

        for(x=0; type_info[x].name; x++) {
                if (strcmp(name,type_info[x].name)==0)
                        return type_info[x].type;
        }
        return DATA_TYPE_UNKNOWN;
}
#endif

void conv_type(int dt,void *d,int dl,int st,void *s,int sl) {
	conv_func_t *func;

	/* Allocate convert temp var */
	dprintf("dest type: %s",dt2name(dt));
	dprintf("src type: %s",dt2name(st));
	if ((st > DATA_TYPE_UNKNOWN && st < DATA_TYPE_MAX) &&
	    (dt > DATA_TYPE_UNKNOWN && dt < DATA_TYPE_MAX)) {
		func = conv_funcs[st][dt];
		if (func) {
			dprintf("calling %s...",conv_func_names[st][dt]);
			func(d,dl,s,sl);
		}
	}
	return;
}

#if 0
struct {
	int type;
	char *p;
} prefixes[] = {
	{ DATA_TYPE_UNKNOWN, "unk" },
	{ DATA_TYPE_CHAR, "chr" },
	{ DATA_TYPE_STRING, "str" },
	{ DATA_TYPE_BYTE, "byte" },
	{ DATA_TYPE_SHORT, "short" },
	{ DATA_TYPE_INT, "int" },
	{ DATA_TYPE_LONG, "long" },
	{ DATA_TYPE_QUAD, "quad" },
	{ DATA_TYPE_FLOAT, "float" },
	{ DATA_TYPE_DOUBLE, "dbl" },
	{ DATA_TYPE_LOGICAL, "log" },
	{ DATA_TYPE_DATE, "date" },
	{ DATA_TYPE_LIST, "list" },
	{ 0, 0 }
}
#endif

char *prefixes[] = {
	"unk",		/* DATA_TYPE_UNKNOWN */
	"chr",		/* DATA_TYPE_CHAR */
	"str",		/* DATA_TYPE_STRING */
	"byte",		/* DATA_TYPE_BYTE */
	"short",	/* DATA_TYPE_SHORT */
	"int",		/* DATA_TYPE_INT */
	"long",		/* DATA_TYPE_LONG */
	"quad",		/* DATA_TYPE_QUAD */
	"float",	/* DATA_TYPE_FLOAT */
	"dbl",		/* DATA_TYPE_DOUBLE */
	"log",		/* DATA_TYPE_LOGICAL */
	"date",		/* DATA_TYPE_DATE */
	"list",		/* DATA_TYPE_list */
	0,
};

#if 0
int main(void) {
	FILE *fp;
	int count;
	register int x,y;

	fp = fopen("conv_type.h","w+");
	if (!fp) {
		perror("fopen conv_type.h");
		return 1;
	}
	for(count=0; prefixes[count]; count++);

	fprintf(fp,"\n");
	fprintf(fp,"#include \"conv_char.h\"\n");
	fprintf(fp,"#include \"conv_string.h\"\n");
	fprintf(fp,"#include \"conv_byte.h\"\n");
	fprintf(fp,"#include \"conv_short.h\"\n");
	fprintf(fp,"#include \"conv_int.h\"\n");
	fprintf(fp,"#include \"conv_long.h\"\n");
	fprintf(fp,"#include \"conv_quad.h\"\n");
	fprintf(fp,"#include \"conv_float.h\"\n");
	fprintf(fp,"#include \"conv_double.h\"\n");
	fprintf(fp,"#include \"conv_log.h\"\n");
	fprintf(fp,"#include \"conv_date.h\"\n");
	fprintf(fp,"#include \"conv_list.h\"\n");
	fprintf(fp,"\n");

	fprintf(fp,"#if DEBUG\n");
	fprintf(fp,"static char *conv_func_names[%d][%d] = {\n",count,count);
	fprintf(fp,"\t{ ");
	for(x=0; x < count; x++) fprintf(fp,"0%s",(x+1 < count ? ", " : " " ));
	fprintf(fp,"},\n");
	for(x=1; prefixes[x]; x++) {
		fprintf(fp,"\t{\t0,\n");
		for(y=1; prefixes[y]; y++) {
		 	fprintf(fp,"\t\t\"%s2%s\",\n",prefixes[x],prefixes[y]);
		}
		fprintf(fp,"\t},\n");
	}
	fprintf(fp,"};\n");
	fprintf(fp,"#endif\n\n");

	fprintf(fp,"typedef void conv_func_t(char *,int,char *,int);\n");
	fprintf(fp,"static conv_func_t *conv_funcs[%d][%d] = {\n",count,count);
	fprintf(fp,"\t{ ");
	for(x=0; x < count; x++) fprintf(fp,"0%s",(x+1 < count ? ", " : " " ));
	fprintf(fp,"},\n");
	for(x=1; prefixes[x]; x++) {
		fprintf(fp,"\t{\t0,\n");
		for(y=1; prefixes[y]; y++) {
			fprintf(fp,"\t\t(conv_func_t *)%s2%s,\n",
				prefixes[x],prefixes[y]);
		}
		fprintf(fp,"\t},\n");
	}
	fprintf(fp,"};\n");
	return 0;
}
/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)strtoq.c	8.1 (Berkeley) 6/4/93";
#endif /* LIBC_SCCS and not lint */

#ifndef lint
static const char rcsid[] =
  "$FreeBSD: src/lib/libc/stdlib/strtoll.c,v 1.5.2.1 2001/03/02 09:45:20 obrien Exp $";
#endif

#include <sys/types.h>

#include <limits.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>

#ifndef LLONG_MAX
#define ULLONG_MAX      0xffffffffffffffffULL
#define LLONG_MAX       0x7fffffffffffffffLL    /* max value for a long long */
#define LLONG_MIN       (-0x7fffffffffffffffLL - 1) /* min for a long long */
#endif

/*
 * Convert a string to a long long integer.
 *
 * Ignores `locale' stuff.  Assumes that the upper and lower case
 * alphabets and digits are each contiguous.
 */
long long
strtoll(nptr, endptr, base)
	const char *nptr;
	char **endptr;
	register int base;
{
	register const char *s;
	register unsigned long long acc;
	register unsigned char c;
	register unsigned long long qbase, cutoff;
	register int neg, any, cutlim;

	/*
	 * Skip white space and pick up leading +/- sign if any.
	 * If base is 0, allow 0x for hex and 0 for octal, else
	 * assume decimal; if base is already 16, allow 0x.
	 */
	s = nptr;
	do {
		c = *s++;
	} while (isspace(c));
	if (c == '-') {
		neg = 1;
		c = *s++;
	} else {
		neg = 0;
		if (c == '+')
			c = *s++;
	}
	if ((base == 0 || base == 16) &&
	    c == '0' && (*s == 'x' || *s == 'X')) {
		c = s[1];
		s += 2;
		base = 16;
	}
	if (base == 0)
		base = c == '0' ? 8 : 10;

	/*
	 * Compute the cutoff value between legal numbers and illegal
	 * numbers.  That is the largest legal value, divided by the
	 * base.  An input number that is greater than this value, if
	 * followed by a legal input character, is too big.  One that
	 * is equal to this value may be valid or not; the limit
	 * between valid and invalid numbers is then based on the last
	 * digit.  For instance, if the range for quads is
	 * [-9223372036854775808..9223372036854775807] and the input base
	 * is 10, cutoff will be set to 922337203685477580 and cutlim to
	 * either 7 (neg==0) or 8 (neg==1), meaning that if we have
	 * accumulated a value > 922337203685477580, or equal but the
	 * next digit is > 7 (or 8), the number is too big, and we will
	 * return a range error.
	 *
	 * Set any if any `digits' consumed; make it negative to indicate
	 * overflow.
	 */
	qbase = (unsigned)base;
	cutoff = neg ? (unsigned long long)-(LLONG_MIN + LLONG_MAX) + LLONG_MAX
	    : LLONG_MAX;
	cutlim = cutoff % qbase;
	cutoff /= qbase;
	for (acc = 0, any = 0;; c = *s++) {
		if (!isascii(c))
			break;
		if (isdigit(c))
			c -= '0';
		else if (isalpha(c))
			c -= isupper(c) ? 'A' - 10 : 'a' - 10;
		else
			break;
		if (c >= base)
			break;
		if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim))
			any = -1;
		else {
			any = 1;
			acc *= qbase;
			acc += c;
		}
	}
	if (any < 0) {
		acc = neg ? LLONG_MIN : LLONG_MAX;
		errno = ERANGE;
	} else if (neg)
		acc = -acc;
	if (endptr != 0)
		*endptr = (char *)(any ? s - 1 : nptr);
	return (acc);
}
#endif
