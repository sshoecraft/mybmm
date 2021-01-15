#ifndef __CONV_H
#define __CONV_H

/*************************************************************************
 *
 * Data types, fields, records, and conversions
 *
 *************************************************************************/

/* Define the data types */
/* XXX ORDER IS IMPORTANT HERE - DO NOT CHANGE */
enum DATA_TYPE {
	DATA_TYPE_UNKNOWN = 0,		/* Unknown */
	DATA_TYPE_CHAR,			/* Array of chars w/no null */
	DATA_TYPE_STRING,		/* Array of chars w/null */
	DATA_TYPE_BYTE,			/* char -- 8 bit */
	DATA_TYPE_SHORT,		/* Short -- 16 bit */
	DATA_TYPE_INT,			/* Integer -- 16 | 32 bit */
	DATA_TYPE_LONG,			/* Long -- 32 bit */
	DATA_TYPE_QUAD,			/* Quadword - 64 bit */
	DATA_TYPE_FLOAT,		/* Float -- 32 bit */
	DATA_TYPE_DOUBLE,		/* Double -- 64 bit */
	DATA_TYPE_LOGICAL,		/* (int) Yes/No,True/False,on/off */
	DATA_TYPE_DATE,			/* (char 23) DD-MMM-YYYY HH:MM:SS.HH */
	DATA_TYPE_LIST,			/* Itemlist */
	DATA_TYPE_MAX			/* Max data type number */
};

/* Special data types */
#ifndef WIN32
typedef char byte;
#else
//#define printf win_printf
#define bcopy(s,d,l) memcpy(d,s,l)
//#define strtoq strtol
typedef unsigned char byte;
#endif
typedef long long myquad_t;

/* Define the field info */
typedef struct {
	char *name;			/* Field name */
	int offset;			/* Field offset */
	int type;			/* Field type */
	int len;			/* Field length */
} field_t;
#define FIELD_SIZE sizeof(field_t)

/* Define the record info */
typedef struct {
	field_t *field;			/* Field info */
	void *data;			/* Record data */
	int size;			/* Record size */
} record_t;
#define RECORD_SIZE sizeof(record_t)

/* Functions */
#ifdef __cplusplus
extern "C" {
#endif

char *dt2name(int);
int name2dt(char *);
void conv_type(int,void *,int,int,void *,int);
#ifdef __cplusplus
}
#endif

#endif /* __CONV_H */
