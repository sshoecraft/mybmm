
#include <dlfcn.h>
#include <errno.h>
#include <sys/types.h>
#include <pwd.h>
#include "mybmm.h"
#include "cfg.h"
#include "battery.h"

int get_mqtt_conf(mybmm_config_t *conf) {
	struct cfg_proctab tab[] = {
		{ "mqtt", "broker", "Broker URL", DATA_TYPE_STRING,&conf->mqtt_broker,sizeof(conf->mqtt_broker), 0 },
		{ "mqtt", "topic", "Topic", DATA_TYPE_STRING,&conf->mqtt_topic,sizeof(conf->mqtt_topic), 0 },
		{ "mqtt", "username", "Broker username", DATA_TYPE_STRING,&conf->mqtt_username,sizeof(conf->mqtt_username), 0 },
		{ "mqtt", "password", "Broker password", DATA_TYPE_STRING,&conf->mqtt_password,sizeof(conf->mqtt_password), 0 },
		CFG_PROCTAB_END
	};

	cfg_get_tab(conf->cfg,tab);
#ifdef DEBUG
	if (debug) cfg_disp_tab(tab,0,1);
#endif
	return 0;
}

int read_config(mybmm_config_t *conf) {
        struct cfg_proctab myconf[] = {
		{ "mybmm", "interval", "Time between updates", DATA_TYPE_INT, &conf->interval, 0, "30" },
		{ "mybmm", "max_charge_amps", "Charge Current", DATA_TYPE_FLOAT, &conf->max_charge_amps, 0, "-1" },
		{ "mybmm", "max_discharge_amps", "Discharge Current", DATA_TYPE_FLOAT, &conf->max_discharge_amps, 0, "-1" },
		{ "mybmm", "system_voltage", "Battery/System Voltage", DATA_TYPE_INT, &conf->system_voltage, 0, "48" },
		{ "mybmm", "battery_chem", "Battery Chemistry", DATA_TYPE_INT, &conf->battery_chem, 0, "1" },
		{ "mybmm", "cells", "Number of battery cells per pack", DATA_TYPE_INT, &conf->cells, 0, "14" },
		{ "mybmm", "cell_low", "Cell low voltage", DATA_TYPE_FLOAT, &conf->cell_low, 0, "-1" },
		{ "mybmm", "cell_crit_low", "Critical cell low voltage", DATA_TYPE_FLOAT, &conf->cell_crit_low, 0, "-1" },
		{ "mybmm", "cell_high", "Cell high voltage", DATA_TYPE_FLOAT, &conf->cell_high, 0, "-1" },
		{ "mybmm", "cell_crit_high", "Critical cell high voltage", DATA_TYPE_FLOAT, &conf->cell_crit_high, 0, "-1" },
		{ "mybmm", "c_rate", "Charge rate", DATA_TYPE_FLOAT, &conf->c_rate, 0, "-1" },
		{ "mybmm", "d_rate", "Discharge rate", DATA_TYPE_FLOAT, &conf->d_rate, 0, "-1" },
		{ "mybmm", "database", "Database Connection", DATA_TYPE_STRING, &conf->db_name, sizeof(conf->db_name), "" },
		/* Forced values */
		{ "mybmm", "capacity", "Battery Capacity", DATA_TYPE_FLOAT, &conf->user_capacity, 0, "-1" },
		{ "mybmm", "charge_voltage", "Charge Voltage", DATA_TYPE_FLOAT, &conf->user_charge_voltage, 0, "-1" },
		{ "mybmm", "charge_amps", "Charge Current", DATA_TYPE_FLOAT, &conf->user_charge_amps, 0, "-1" },
		{ "mybmm", "discharge_voltage", "Discharge Voltage", DATA_TYPE_FLOAT, &conf->user_discharge_voltage, 0, "-1" },
		{ "mybmm", "discharge_amps", "Discharge Current", DATA_TYPE_FLOAT, &conf->user_discharge_amps, 0, "-1" },
		{ "mybmm", "soc", "Force State of Charge", DATA_TYPE_FLOAT, &conf->user_soc, 0, "-1.0" },
		CFG_PROCTAB_END
	};

	if (conf->filename) {
		conf->cfg = cfg_read(conf->filename);
		dprintf(3,"cfg: %p\n", conf->cfg);
		if (!conf->cfg) {
			printf("error: unable to read config file '%s': %s\n", conf->filename, strerror(errno));
			return 1;
		}
	}

	cfg_get_tab(conf->cfg,myconf);
#ifdef DEBUG
	if (debug) cfg_disp_tab(myconf,0,1);
#endif

#ifdef MQTT
	if (get_mqtt_conf(conf)) return 1;
#endif

	dprintf(1,"db_name: %s\n", conf->db_name);
//	if (strlen(conf->db_name)) db_init(conf,conf->db_name);

#if 0
	conf->dlsym_handle = dlopen(0,RTLD_LAZY);
	if (!conf->dlsym_handle) {
		printf("error getting dlsym_handle: %s\n",dlerror());
		return 1;
	}
	dprintf(3,"dlsym_handle: %p\n",conf->dlsym_handle);
#endif


	/* Init battery config */
	if (battery_init(conf)) return 1;

	/* Init inverter */
	if (inverter_init(conf)) return 1;

	/* Init battery pack */
	if (pack_init(conf)) return 1;

	/* If config is dirty, write it back out */
	dprintf(1,"conf: dirty? %d\n", mybmm_check_state(conf,MYBMM_CONFIG_DIRTY));
	if (mybmm_check_state(conf,MYBMM_CONFIG_DIRTY)) {
		cfg_write(conf->cfg);
		mybmm_clear_state(conf,MYBMM_CONFIG_DIRTY);
	}

	return 0;
}

int reconfig(mybmm_config_t *conf) {
	dprintf(1,"destroying lists...\n");
	list_destroy(conf->modules);
	list_destroy(conf->packs);
	dprintf(1,"destroying threads...\n");
	pthread_cancel(conf->inverter_tid);
	pthread_cancel(conf->pack_tid);
	dprintf(1,"creating lists...\n");
	conf->modules = list_create();
	conf->packs = list_create();
	free(conf->cfg);
	return read_config(conf);
}

mybmm_config_t *get_config(char *filename) {
	mybmm_config_t *conf;

	conf = calloc(1,sizeof(mybmm_config_t));
	if (!conf) {
		perror("malloc mybmm_config_t");
		return 0;
	}
	conf->modules = list_create();
	conf->packs = list_create();

	conf->filename = filename;
	if (read_config(conf)) {
		free(conf);
		return 0;
	}
	return conf;
}
