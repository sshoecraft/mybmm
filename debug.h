
#ifndef __DEBUG_H
#define __DEBUG_H

#include <stdio.h>

extern int debug;
#ifdef DEBUG
#define dprintf(level, format, args...) { if (debug >= level) printf("%s(%d): " format,__FUNCTION__,__LINE__, ## args); }
#else
#define dprintf(level,format,args...) /* noop */
#endif

#endif /* __DEBUG_H */
