
#include <string.h>
#include "fileinfo.h"

char *os_fnmerge(char *name, FILEINFO *info) {

	if (!name) return 0;

	name[0] = 0;
	if (!info) return(name);
#if 0
	if (pi->node) {
		strncat(filename,pi->node,pi->node_len);
		if (filename[strlen(filename)-1] != ':')
			strcat(filename,":");
	}
	if (pi->dir) {
		strncat(filename,pi->dir,pi->dir_len);
		if (filename[strlen(filename)-1] != '/')
			strcat(filename,"/");
	}
	if (pi->name) strncat(filename,pi->name,pi->name_len);
	if (pi->type) strncat(filename,pi->type,pi->type_len);
#endif

	/* Build the return string */
#ifdef netware	
	if (info->node) {
		strncat(name,info->node,info->node_len);
		strcat(name,"/");
	}		
#else	
	if (info->node) strncat(name,info->node,info->node_len);
#endif	
	if (info->dev) strncat(name,info->dev,info->dev_len);
	if (info->dir) {
#ifndef vms
//		if (info->dir[0] != SLASH_CHAR && info->dir != cwd.dir)
//			strncat(name,cwd.dir,cwd.dir_len);
		strncat(name,info->dir,info->dir_len);
		if (name[strlen(name)-1]!=SLASH_CHAR)
			strcat(name,SLASH_STRING);
#else
		strncat(name,info->dir,info->dir_len);
#endif
	}
	if (info->name) strncat(name,info->name,info->name_len);
	if (info->type) strncat(name,info->type,info->type_len);
#ifdef vms
	if (info->ver) strncat(name,info->ver,info->ver_len);
#endif
	return(name);
}
