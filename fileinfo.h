#ifndef __FILEINFO_H
#define __FILEINFO_H

struct FileInfo {
        char *node;                     /* Node */
        int node_len;                   /* Node length */
        char *dev;                      /* Device */
        int dev_len;                    /* Device length */
        char *dir;                      /* Directory */
        int dir_len;                    /* Directory length */
        char *name;                     /* Name */
        int name_len;                   /* Name length */
        char *type;                     /* Type (extension) */
        int type_len;                   /* Type length */
        char *ver;                      /* Version (vms or ISO only) */
        int ver_len;                    /* Version length */
};
#define FILEINFO_SIZE sizeof(struct FileInfo)
typedef struct FileInfo FILEINFO;

char *os_fnparse(char *filespec,char *defspec,char *field);
void os_fnsplit(FILEINFO *info,char *spec);
char *os_fnmerge(char *name, FILEINFO *pi);

/* Define the directory seperator in filenames */
#ifdef vms
#define SLASH_CHAR ''
#define SLASH_STRING ""
#define OS_STATUS_SUCCESS       1
#define OS_STATUS_ERROR         2
#else
#define OS_STATUS_SUCCESS       0
#define OS_STATUS_ERROR         1
#ifdef MS_DOS
#define SLASH_CHAR '\\'
#define SLASH_STRING "\\"
#else
#define SLASH_CHAR '/'
#define SLASH_STRING "/"
#endif
#endif

#endif
