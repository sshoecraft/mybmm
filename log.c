
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#ifdef DEBUG
#undef DEBUG
#endif
//#define DEBUG 1

#include "mybmm.h"
#include "log.h"

#define DEFAULT_OPTIONS		LOG_INFO|LOG_WARNING|LOG_ERROR|LOG_SYSERR
static FILE *logfp = (FILE *) 0;
static int logopts = DEFAULT_OPTIONS;
static char logfile[256];
static char message[32767];

#if 0
static char *month_names[12] = {
	"JAN","FEB","MAR","APR","MAY","JUN",
	"JUL","AUG","SEP","OCT","NOV","DEC"
};
#endif

char *os_fnparse(char *filespec,char *defspec,char *field);

int _os_get_times(int type, char *dt, int len) {
	struct tm *tptr;
	time_t t;

	DPRINTF("type: %d, dt: %p, len: %d\n", type, dt, len);
	*dt = 0;
	if (len < (type == 1 ? 24 : 15)) {
		DPRINTF("invalid length, returning");
		return 1;
	}

	/* Fill the tm struct */
	t = time(NULL);
	tptr = 0;
	DPRINTF("getting localtime\n");
	tptr = localtime(&t);
	if (!tptr) {
		DPRINTF("unable to get localtime!\n");
		return 1;
	}

	/* Set month to 1 if month is out of range */
	if (tptr->tm_mon < 0 || tptr->tm_mon > 11) tptr->tm_mon = 0;

	/* Fill string with yyyymmddhhmmss */
	sprintf(dt,"%04d-%02d-%02d %02d:%02d:%02d",
		1900+tptr->tm_year,tptr->tm_mon+1,tptr->tm_mday,
		tptr->tm_hour,tptr->tm_min,tptr->tm_sec);

	return 0;
}
#define os_get_logtimes(d,l) _os_get_times(2,d,l);

#if DEBUG
static struct _opt_info {
	int type;
	char *name;
} optlist[] = {
	{ LOG_CREATE, "CREATE" },
	{ LOG_TIME, "TIME" },
	{ LOG_STDERR, "STDERR" },
	{ LOG_INFO, "INFO" },
	{ LOG_VERBOSE, "VERBOSE" },
	{ LOG_WARNING, "WARNING" },
	{ LOG_ERROR, "ERROR" },
	{ LOG_SYSERR, "SYSERR" },
	{ LOG_DEBUG, "DEBUG" },
	{ LOG_DEBUG2, "DEBUG2" },
	{ LOG_DEBUG3, "DEBUG3" },
	{ LOG_DEBUG4, "DEBUG4" },
	{ 0,0 }
};

static __inline void _dispopts(int type) {
	register struct _opt_info *opt;
	register char *ptr;

	message[0] = 0;
	sprintf(message,"log options:");
	ptr = message + strlen(message);
	for(opt = optlist; opt->name; opt++) {
		if (type & opt->type) {
			sprintf(ptr," %s",opt->name);
			ptr += strlen(ptr);
		}
	}
	if (logfp) fprintf(logfp,"%s\n",message);
	return;
}
#endif

int log_open(char *ident,char *filename,int opts) {

#if DEBUG
	DPRINTF("ident: %p, filename: %p,opts: %x\n",ident,filename,opts);
	_dispopts(opts);
	exit(0);
#endif

	if (filename) {
		char *op;

		strcpy(logfile,os_fnparse(filename,0,0));
		DPRINTF("logging to file: %s\n",logfile);

		/* Open the file */
		op = (opts & LOG_CREATE ? "w+" : "a+");
		logfp = fopen(logfile,op);
		if (!logfp) {
			perror("log_open: unable to create logfile");
			return 1;
		}
	} else if (opts & LOG_WX) {
		DPRINTF("logging to wx\n");
		logfp = (FILE *) ident;
	} else if (opts & LOG_STDERR) {
		DPRINTF("logging to stderr\n");
		logfp = stderr;
	} else {
		DPRINTF("logging to stdout\n");
		logfp = stdout;
	}
	logopts = opts;

	DPRINTF("log is opened.\n");
	return 0;
}

#if 0
static void mytrim(char *string) {
	register char *src,*dest;

	DPRINTF("before: >>>%s<<<\n",string);

	/* Trim the front */
	src = string;
	while(isspace((int)*src) && *src != '\t') src++;
	dest = string;
	while(*src != '\0') *dest++ = *src++;

	/* Trim the back */
	*dest-- = '\0';
	while((dest >= string) && isspace((int)*dest)) dest--;
	*(dest+1) = '\0';

	DPRINTF("after: >>>%s<<<\n",string);
}
#endif

typedef void (*func_t)(char *);

/* XXX Cannot use _ANY_ external func calls that may call log_write (DUH!) */
int log_write(int type,char *format,...) {
	va_list ap;
	char dt[32],error[128];
	register char *ptr;

	/* Make sure log_open was called */
	if (!logfp) return 1;

	/* Do we even log this type? */
	DPRINTF("logopts: %0x, type: %0x\n",logopts,type);
	if ( (logopts | type) != logopts) return 0;

	/* get the error text asap before it's gone */
	if (type & LOG_SYSERR) {
		error[0] = 0;
		strncat(error,strerror(errno),sizeof(error));
	}

	/* Prepend the time? */
	ptr = message;
	if (logopts & LOG_TIME || type & LOG_TIME) {
		DPRINTF("prepending time...\n");
		os_get_logtimes(dt,sizeof(dt));
		strcat(dt,"  ");
		sprintf(ptr,"%s",dt);
		ptr += strlen(ptr);
	}

	/* If it's a warning, prepend warning: */
	if (type & LOG_WARNING) {
		DPRINTF("prepending warning...\n");
		sprintf(ptr,"warning: ");
		ptr += strlen(ptr);
	}

	/* If it's an error, prepend error: */
	else if ((type & LOG_ERROR) || (type & LOG_SYSERR)) {
		DPRINTF("prepending error...\n");
		sprintf(ptr,"error: ");
		ptr += strlen(ptr);
	}

	/* Build the rest of the message */
	DPRINTF("adding message...\n");
	va_start(ap,format);
	vsprintf(ptr,format,ap);
	va_end(ap);

	/* Trim */
	trim(message);

	/* If it's a system error, concat the system message */
	if (type & LOG_SYSERR) {
		DPRINTF("adding error text...\n");
		strcat(message,": ");
		strcat(message, error);
	}

	/* Strip all CRs and LFs */
	DPRINTF("stripping newlines...\n");
	for(ptr = message; *ptr; ptr++) {
		if (*ptr == '\n' || *ptr == '\r')
			strcpy(ptr,ptr+1);
	}

	/* Write the message */
	DPRINTF("message: %s\n",message);
	if (logopts & LOG_WX) {
		func_t func = (func_t) logfp;
		func(message);
	} else {
		fprintf(logfp,"%s\n",message);
		fflush(logfp);
	}
	return 0;
}

void log_close(void) { 
	if (logfp) fclose(logfp);
	logfp = 0;
	logfile[0] = 0;
	logopts = 0;
	return; 
}

#if 0
//	strcpy(message,stredit(message,"TRIM"));
	ptr = &message[strlen(message)-1];
	while(*ptr == '\n' || *ptr == '\r') {
		*ptr = 0;
		ptr--;
	}
int log_debug(char *format,...) {
	char msg[4096];
	va_list ap;

	va_start(ap,format);
	vsprintf(msg,format,ap);
	va_end(ap);
	return DLOG(LOG_DEBUG,msg);
}

char *log_nextname(void) {
	static char filename[13];
	char vfile[256],prefix[7],*ptr;

	/* Get the version/control filename */
	vfile[0] = 0;
	strcpy(vfile,home_dir);
	strcat(vfile,program_name);
	strcat(vfile,".ctl");
	DPRINTF("log_nextname: vfile: %s\n",vfile);

	/* Restrict program name to 6 chars */
	prefix[0] = 0;
	strncat(prefix,program_name,sizeof(prefix)-1);
	DPRINTF("log_nextname: prefix: %s\n",prefix);

	/* Get the next name */
	ptr = get_seq_filename(vfile,prefix,".log");
	if (!ptr) {
		perror("log_nextname: get_seq_filename");
		return 0;
	}
	strcpy(filename,ptr);

	return filename;
}
#endif
