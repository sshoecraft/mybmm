
#ifndef __MYBMM_CONFIG_H
#define __MYBMM_CONFIG_H

#include <stdint.h>
#include <pthread.h>
#include "list.h"
#include "worker.h"

struct mybmm_inverter;
typedef struct mybmm_inverter mybmm_inverter_t;

struct mybmm_pack;
typedef struct mybmm_pack mybmm_pack_t;

struct mybmm_config {
	char username[64];
	char home_dir[256];
	char *filename;			/* Config filename */
	void *logfp;			/* Log filehandle */
	char db_name[32];		/* DB Name */
	void *dlsym_handle;		/* Image handle */
	list modules;			/* Modules */
	mybmm_inverter_t *inverter;	/* Inverter */
	pthread_t inverter_tid;		/* Inverter Thread ID */
	list packs;			/* Packs */
	pthread_t pack_tid;		/* Pack thread ID */
	worker_pool_t *pack_pool;	/* Pack worker pool */
	int interval;			/* Check interval */
	int system_voltage;		/* System Voltage (defaults to 48) */
	int battery_chem;		/* Battery type (0=Li-ion, 1=LifePO4, 2=Titanate) */
	float battery_voltage;		/* Battery Voltage  */
	float battery_current;		/* Total amount of power into/out of battery */
	int cells;			/* Number of cells per battery pack */
	float cell_low;			/* Cell discharge low cutoff */
	float cell_crit_low;		/* Cell critical low */
	float cell_high;		/* Cell charge high cutoff */
	float cell_crit_high;		/* Cell critical high */
	float capacity;			/* Total capacity, in AH (all packs) */
	float c_rate;			/* Charge current rate */
	float d_rate;			/* Discharge current rate */
	float kwh;			/* Calculated kWh */
	float soc;			/* State of Charge */
	float soh;			/* State of Health */
	float input_current;		/* Power from sources */
	float output_current;		/* Power to loads/batt */
	float capacity_remain;		/* Remaining capacity */
	float max_charge_amps;		/* From inverter */
	float max_discharge_amps;	/* From inverter */
	float discharge_voltage;	/* Calculated: cell_low * cells */
	float discharge_amps;		/* Calculated */
	float charge_voltage;		/* Calculated: cell_high * cells */
	float charge_amps;		/* Calculated */
	float user_capacity;		/* User-specified capacity */
	float user_charge_voltage;	/* User-specified charge voltage */
	float user_charge_amps;		/* User-specified charge amps */
	float user_discharge_voltage;	/* User-specified discharge voltage */
	float user_discharge_amps;	/* User-specified discharge amps */
	float user_soc;			/* Forced State of Charge */
	void *cfg;			/* Config file handle */
	uint16_t state;			/* States */
	uint16_t capabilities;		/* Capabilities */
};
typedef struct mybmm_config mybmm_config_t;

/* States */
#define MYBMM_STATE_CONFIG_DIRTY	0x01		/* Config has been updated & needs written */

/* Capabilities */
#define MYBMM_CHARGE_CONTROL		0x01		/* All packs have ability to start/stop charging */
#define MYBMM_DISCHARGE_CONTROL		0x02		/* All packs have ability to start/stop discharging */
#define MYBMM_BALANCE_CONTROL		0x04		/* All packs have ability to start/stop balancing */

mybmm_config_t *get_config(char *);
int reconfig(mybmm_config_t *conf);

#endif /* __MYBMM_CONFIG_H */
