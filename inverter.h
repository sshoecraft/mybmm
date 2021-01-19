
#ifndef __MYBMM_INVERTER_H
#define __MYBMM_INVERTER_H

#include "config.h"

struct mybmm_inverter {
	char name[MYBMM_PACK_NAME_LEN];	/* Inverter name */
	char uuid[37];			/* Inverter UUID */
	char type[32];			/* Inverter type */
	char transport[32];		/* Transport name */
	char target[32];		/* Device/Interface/Address */
	char params[64];		/* Inverter-specific params */
	unsigned char state;		/* Inverter State */
	float battery_voltage;		/* Really? I need to comment this? */
	float battery_current;		/* batt watts */
	float grid_current;		/* Grid/Gen watts */
	float load_current;		/* loads watts */
	float pv_current;		/* pv/wind/caes watts */
	void *handle;			/* Inverter Handle */
	mybmm_module_open_t open;	/* Inverter Open */
	mybmm_module_control_t control;	/* Inverter Control */
	mybmm_module_read_t read;	/* Inverter Read */
	mybmm_module_write_t write;	/* Inverter Write */
	mybmm_module_close_t close;	/* Inverter Close */
	unsigned short capabilities;	/* Capability bits */
//	int failed;			/* Failed to update count */
};
typedef struct mybmm_inverter mybmm_inverter_t;

/* States */
#define MYBMM_INVERTER_STATE_UPDATED	0x01
#define MYBMM_INVERTER_STATE_RUNNING	0x02
#define MYBMM_INVERTER_STATE_GRID	0x04
#define MYBMM_INVERTER_STATE_GEN	0x08

/* Capabilities */
#define MYBMM_INVERTER_GRID_CONTROL	0x01
#define MYBMM_INVERTER_GEN_CONTROL	0x02
#define MYBMM_INVERTER_POWER_CONTROL	0x04

int inverter_init(mybmm_config_t *conf);
int inverter_start_update(mybmm_config_t *conf);
int inverter_read(mybmm_inverter_t *inv);
int inverter_write(mybmm_inverter_t *inv);

#endif /* __MYBMM_INVERTER_H */
