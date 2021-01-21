
#include <unistd.h>
#include <string.h>

#ifdef DEBUG
#undef DEBUG
#endif
//#define DEBUG 1

#include "log.h"
#include "fileinfo.h"
#include "debug.h"

#define SLASH_STRING "/"
#define SLASH_CHAR '/'

/*
** Fields...
	NODE            Node name               (VMS and UNIX)
	DEVICE          Device name		(Except UNIX)
	DIRECTORY       Directory name
	NAME            File name
	TYPE            File type
	VERSION         File version number      (VMS only)
*/

#if DEBUG
static void _printit(char *name, char *string, int len) {
	char temp[255];

	temp[0] = 0;
	strncat(temp,string,len);
	DLOG(LOG_DEBUG,"%s(%d): %s\n",name,len,temp);
	return;
}
#define DLOGF(n,s,l) _printit(n,s,l)
#else
#define DLOGF(n,s,l) /* noop */
#endif

char *stredit(char *string, char *list);

char *os_fnparse(char *filespec,char *defspec,char *field) {
	static char return_string[255];
#if vms
	char fileexp[255],defexp[255];
#else
	char temp[255];
#endif
	char *fspec,*dspec,*ptr;
	FILEINFO file,def,cwd;

	fspec = (filespec ? filespec : "");
	dspec = (defspec ? defspec : "");
	DLOG(LOG_DEBUG,"Filespec: %s\n",fspec);
	DLOG(LOG_DEBUG,"Defspec: %s\n",dspec);
	if (field) DLOG(LOG_DEBUG,"Field: %s\n",field);

#if vms
	_vmsparse(fspec,fileexp,255);
	os_fnsplit(&file,fileexp);
	_vmsparse(dspec,defexp,255);
	os_fnsplit(&def,defexp);
#else
	os_fnsplit(&file,fspec);
	os_fnsplit(&def,dspec);
#endif

	/* Now add the defspec into the filespec */
	if (file.node) DLOGF("fnode",file.node,file.node_len);
	if (def.node) DLOGF("dnode",def.node,def.node_len);
	if (!file.node_len && def.node_len) {
		file.node = def.node;
		file.node_len = def.node_len;
	}
	if (file.dev) DLOGF("fdev",file.dev,file.dev_len);
	if (def.dev) DLOGF("ddev",def.dev,def.dev_len);
	if (!file.dev_len && def.dev_len) {
		file.dev = def.dev;
		file.dev_len = def.dev_len;
	}
	if (file.dir) DLOGF("fdir",file.dir,file.dir_len);
	if (def.dir) DLOGF("ddir",def.dir,def.dir_len);
	if (!file.dir_len && def.dir_len) {
		file.dir = def.dir;
		file.dir_len = def.dir_len;
	}
	if (file.name) DLOGF("fname",file.name,file.name_len);
	if (def.name) DLOGF("dname",def.name,def.name_len);
	if (!file.name_len && def.name_len) {
		file.name = def.name;
		file.name_len = def.name_len;
	}
	if (file.type) DLOGF("ftype",file.type,file.type_len);
	if (def.type) DLOGF("dtype",def.type,def.type_len);
#if vms
	if (file.type_len == 1 && def.type_len > 1) {
#else
	if (!file.type_len && def.type_len) {
#endif
		file.type = def.type;
		file.type_len = def.type_len;
	}
	if (file.ver) DLOGF("fver",file.ver,file.ver_len);
	if (def.ver) DLOGF("dver",def.ver,def.ver_len);
#if vms
	if (file.ver_len == 1 && def.ver_len > 1) {
#else
	if (!file.ver_len && def.ver_len) {
#endif
		file.ver = def.ver;
		file.ver_len = def.ver_len;
	}

	return_string[0] = 0;
	if (field) {
		ptr = stredit((char *)field,"TRIM,UPCASE");
		if (strncmp(ptr,"NODE",4)==0 && file.node)
			strncat(return_string,file.node,file.node_len);
		else if (strncmp(ptr,"DEV",3)==0 && file.dev)
			strncat(return_string,file.dev,file.dev_len);
		else if (strncmp(ptr,"DIR",3)==0 && file.dir)
			strncat(return_string,file.dir,file.dir_len);
		else if (strncmp(ptr,"NAME",4)==0 && file.name)
			strncat(return_string,file.name,file.name_len);
		else if (strncmp(ptr,"TYPE",4)==0 && file.type)
			strncat(return_string,file.type,file.type_len);
		else if (strncmp(ptr,"VER",3)==0 && file.ver)
			strncat(return_string,file.ver,file.ver_len);
	}
	else {
#ifndef vms
		/* No field requested.  Return full specification */
		getcwd(temp,sizeof temp);
		strcat(temp,SLASH_STRING);

		/* Split the current dir info */
		os_fnsplit(&cwd,temp);

		/* Add cwd into file info */
		if (file.node) DLOGF("fnode",file.node,file.node_len);
		if (cwd.node) DLOGF("cnode",cwd.node,cwd.node_len);
		if (!file.node_len && cwd.node_len) {
			file.node = cwd.node;
			file.node_len = cwd.node_len;
		}
		if (file.dev) DLOGF("fdev",file.dev,file.dev_len);
		if (cwd.dev) DLOGF("cdev",cwd.dev,cwd.dev_len);
		if (!file.dev_len && cwd.dev_len) {
			file.dev = cwd.dev;
			file.dev_len = cwd.dev_len;
		}

		/* If filespec was ./<something> or .\<something>, remove it */
		if (file.dir_len) {
			if (strncmp(file.dir,"./",file.dir_len)==0)
				file.dir_len = 0;
			else if (strncmp(file.dir,".\\",file.dir_len)==0)
				file.dir_len = 0;
		}
		if (file.dir) DLOGF("fdir",file.dir,file.dir_len);
		if (cwd.dir) DLOGF("cdir",cwd.dir,cwd.dir_len);
		if (!file.dir_len && cwd.dir_len) {
			file.dir = cwd.dir;
			file.dir_len = cwd.dir_len;
		} else if (strncmp(file.dir,"./",file.dir_len)==0) {
			file.dir = cwd.dir;
			file.dir_len = cwd.dir_len;
		}

#if 0 /* This part is just plain stupidity */
		if (file.name) DLOGF("fname",file.name,file.name_len);
		if (cwd.name) DLOGF("cname",cwd.name,cwd.name_len);
		if (!file.name_len && cwd.name_len) {
			file.name = cwd.name;
			file.name_len = cwd.name_len;
		}
		if (file.type) DLOGF("ftype",file.type,file.type_len);
		if (cwd.type) DLOGF("ctype",cwd.type,cwd.type_len);
		if (!file.type_len && cwd.type_len) {
			file.type = cwd.type;
			file.type_len = cwd.type_len;
		}
		if (file.ver) DLOGF("fver",file.ver,file.ver_len);
		if (cwd.ver) DLOGF("cver",cwd.ver,cwd.ver_len);
		if (!file.ver_len && cwd.ver_len) {
			file.ver = cwd.ver;
			file.ver_len = cwd.ver_len;
		}
#endif
#endif

		/* Build the return string */
#ifdef netware	
		if (file.node) {
			strncat(return_string,file.node,file.node_len);
			strcat(return_string,"/");
		}		
#else	
		if (file.node) strncat(return_string,file.node,file.node_len);
#endif	
		if (file.dev) strncat(return_string,file.dev,file.dev_len);
		if (file.dir) {
#ifndef vms
			if (file.dir[0] != SLASH_CHAR && file.dir != cwd.dir)
				strncat(return_string,cwd.dir,cwd.dir_len);
			strncat(return_string,file.dir,file.dir_len);
			if (return_string[strlen(return_string)-1]!=SLASH_CHAR)
				strcat(return_string,SLASH_STRING);
#else
			strncat(return_string,file.dir,file.dir_len);
#endif
		}
		if (file.name) strncat(return_string,file.name,file.name_len);
		if (file.type) strncat(return_string,file.type,file.type_len);
		if (file.ver) strncat(return_string,file.ver,file.ver_len);
	}
	DLOG(LOG_DEBUG,"Return string: %s\n",return_string);
	return(return_string);
}
