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

#endif
