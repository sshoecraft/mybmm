
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>
#include <dlfcn.h>
#include <sys/signal.h>
#include "mybmm.h"
#include "log.h"

int debug = 0;

void display(struct mybmm_config *conf);

volatile uint8_t reconf;
void usr1_handler(int signo) {
	if (signo == SIGUSR1) {
		printf("reconf!\n");
		reconf = 1;
	}
}

void usage(char *name) {
	printf("usage: %s [-acjJrwlh] [-f filename] [-b <bluetooth mac addr | -i <ip addr>] [-o output file]\n",name);
	printf("arguments:\n");
#ifdef DEBUG
	printf("  -d <#>		debug output\n");
#endif
	printf("  -h		this output\n");
	printf("  -t <transport:target> transport & target\n");
	printf("  -e <opts>	transport-specific opts\n");
}

int main(int argc, char **argv) {
	int opt;
	char type[MYBMM_MODULE_NAME_LEN+1],transport[MYBMM_MODULE_NAME_LEN+1],target[MYBMM_TARGET_LEN+1],*opts;
	mybmm_config_t *conf;
	mybmm_pack_t *pp;
	worker_pool_t *pool;
	sigset_t set;

	opts = 0;
	type[0] = transport[0] = target[0] = 0;
	while ((opt=getopt(argc, argv, "d:p:e:h")) != -1) {
		switch (opt) {
		case 'd':
			debug=atoi(optarg);
			break;
                case 'p':
			strncpy(type,strele(0,":",optarg),sizeof(type)-1);
			strncpy(transport,strele(1,":",optarg),sizeof(transport)-1);
			strncpy(target,strele(2,":",optarg),sizeof(target)-1);
			if (!strlen(type) || !strlen(transport) || !strlen(target)) {
				printf("error: format is type:transport:target\n");
				usage(argv[0]);
				return 1;
			}
			break;
		case 'e':
			opts = optarg;
			break;
		case 'h':
		default:
			usage(argv[0]);
			exit(0);
                }
	}

	/* Create the config */
	conf = calloc(sizeof(*conf),1);
	if (!conf) {
		perror("calloc conf");
		return 1;
	}
	conf->modules = list_create();
	conf->packs = list_create();

	printf("opening log...\n");
	log_open("mybmm","mybmm.log",LOG_TIME|LOG_INFO|LOG_WARNING|LOG_ERROR|LOG_SYSERR|LOG_DEBUG);
	log_write(LOG_DEBUG,"testing");
	printf("exiting...\n");

	/* Get config */
	conf = get_config("mybmm.conf");
	dprintf(4,"conf: %p\n", conf);
	if (!conf) return 1;

	/* Ignore SIGPIPE */
        sigemptyset(&set);
        sigaddset(&set, SIGPIPE);
        sigprocmask(SIG_BLOCK, &set, NULL);

	/* USR causes config re-read (could also check file modification time) */
	reconf = 0;
        signal(SIGUSR1, usr1_handler);

while(1) {
	inverter_read(conf->inverter);

	pack_update_all(conf,15);

	/* Set initial values */
	if (conf->user_charge_voltage) {
		conf->charge_voltage = conf->user_charge_voltage;
	} else {
		conf->charge_voltage = conf->cell_high * conf->cells;
	}
	if (conf->user_discharge_voltage) {
		conf->discharge_voltage = conf->user_discharge_voltage;
	} else {
		conf->discharge_voltage = conf->cell_low * conf->cells;
	}
	dprintf(0,"Charge voltage: %.1f, Discharge voltage: %.1f\n", conf->charge_voltage, conf->discharge_voltage);

	/* Charge Voltage / SOC */
	dprintf(1,"Battery_voltage: %.1f\n", conf->battery_voltage);
	if (conf->user_soc >= 0.0) {
		conf->soc = conf->user_soc;
	} else {
		conf->soc = ( ( conf->battery_voltage - conf->discharge_voltage) / (conf->charge_voltage - conf->discharge_voltage) ) * 100.0;
	}

	dprintf(1,"Starting main loop...\n");
	reconf = 0;
		display(conf);

#if 0
		dprintf(0,"Capacity: %.1f\n", conf->capacity);
		conf->kwh = (conf->capacity * conf->system_voltage) / 1000.0;
		dprintf(0,"kWh: %.1f\n", conf->kwh);
#endif

		conf->soh = 100.0;

		/* Charge/Discharge amps */
//		conf->charge_amps = ((conf->c_rate * conf->capacity) * 2) / 3;
		conf->charge_amps = 12;
//		conf->discharge_amps = conf->c_rate * conf->capacity;
		conf->discharge_amps = 30.0;
		dprintf(0,"Charge amps: %.1f, Discharge amps: %.1f\n", conf->charge_amps, conf->discharge_amps);

	dprintf(0,"SoC: %.1f\n", conf->soc);
	inverter_write(conf->inverter);
	sleep(10);
}

	return 0;
}
