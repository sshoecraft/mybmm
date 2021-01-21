
#ifndef __UTILS_H
#define __UTILS_H

#include "cfg.h"
#include "fileinfo.h"

void bindump(char *label,void *bptr,int len);
void _bindump(long offset,void *bptr,int len);
char *trim(char *);
char *strele(int num,char *delimiter,char *string);
void os_fnsplit(FILEINFO *info,char *spec);
char *os_fnparse(char *filespec,char *defspec,char *field);
int is_ip(char *);

#endif
