
#include <string.h>

#ifdef DEBUG
#undef DEBUG
#endif
//#define DEBUG 1

#include "log.h"
#include "fileinfo.h"
#include "debug.h"

void os_fnsplit(FILEINFO *info,char *spec) {
	register char *ptr,*start,*end;

	if (!info) return;
	info->node = info->dev = info->dir = 0;
	info->name = info->type = info->ver = 0;
	info->node_len = info->dev_len = info->dir_len = 0;
	info->name_len = info->type_len = info->ver_len = 0;
	if (!spec) return;

	DLOG(LOG_DEBUG,"spec: %s\n",spec);

	/* Setup the end ptr */
	start = spec;
	end = spec + strlen(spec)-1;

	/* Get the node */
	for(ptr = start; ptr <= end; ptr++) {
		if (*ptr == ':') {
			info->node = start;
			info->node_len = ptr - start + 1;
			start = ptr+1;
			break;
		}
	}

	/* Get the dir */
	for(ptr = end; ptr >= start; ptr--) {
		if (*ptr == '/') {
			info->dir = start;
			info->dir_len = ptr - start + 1;
			start = ptr+1;
			break;
		}
	}

	/* Get the type */
	for(ptr = end; ptr >= start; ptr--) {
		if (*ptr == '.') {
			info->type = ptr;
			info->type_len = end - ptr + 1;
			end = ptr-1;
			break;
		}
	}

	/* Name is in-between start and end now :) */
	info->name_len = end - start + 1;
	if (info->name_len) info->name = start;

#if DEBUG
	{
		char temp[255];

		DLOG(LOG_DEBUG,"Parsed info:\n");
		if (info->node) {
			temp[0] = 0;
			strncat(temp,info->node,info->node_len);
			DLOG(LOG_DEBUG,"\tNode(%d): %s\n",info->node_len,temp);
		}
		if (info->dev) {
			temp[0] = 0;
			strncat(temp,info->dev,info->dev_len);
			DLOG(LOG_DEBUG,"\tDev(%d): %s\n",info->dev_len,temp);
		}
		if (info->dir) {
			temp[0] = 0;
			strncat(temp,info->dir,info->dir_len);
			DLOG(LOG_DEBUG,"\tDir(%d): %s\n",info->dir_len,temp);
		}
		if (info->name) {
			temp[0] = 0;
			strncat(temp,info->name,info->name_len);
			DLOG(LOG_DEBUG,"\tName(%d): %s\n",info->name_len,temp);
		}
		if (info->type) {
			temp[0] = 0;
			strncat(temp,info->type,info->type_len);
			DLOG(LOG_DEBUG,"\tType(%d): %s\n",info->type_len,temp);
		}
		if (info->ver) {
			temp[0] = 0;
			strncat(temp,info->ver,info->ver_len);
			DLOG(LOG_DEBUG,"\tVer(%d): %s\n",info->ver_len,temp);
		}
	}
#endif
	return;
}
