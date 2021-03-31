
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
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>
#include <dlfcn.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <pwd.h>
#include "mybmm.h"
#include "battery.h"
#include "mqtt.h"

#define DEBUG_STARTUP 0

int debug = 0;

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
	printf("  -b <type,capacity> specify battery chem (1=lion, 2=lfp, 3=lto)\n");
#ifdef DEBUG
	printf("  -d <#>		debug output\n");
#endif
	printf("  -c <filename> specify configuration file\n");
	printf("  -l <filename> specify log file\n");
	printf("  -h		this output\n");
	printf("  -i <type:transport:target> specify inverter");
	printf("  -p <type:transport:target> specify cell monitor\n");
}

#define HOURS(n) (n * 3600)

float pct(float val1, float val2) {
	float val;

	dprintf(1,"val1: %.3f, val: %.3f\n", val1, val2);
	val = 100 - ((val2 / val1)  * 100.0);
	dprintf(1,"val: %.3f\n", val);
	return val;
}

static mybmm_config_t *init(int argc, char **argv) {
	int logopts = LOG_TIME|LOG_INFO|LOG_WARNING|LOG_ERROR|LOG_SYSERR;
	int opt,battery_chem,battery_cap;
	char inv_type[MYBMM_MODULE_NAME_LEN+1],inv_transport[MYBMM_MODULE_NAME_LEN+1],inv_target[MYBMM_TARGET_LEN+1];
	char pack_type[MYBMM_MODULE_NAME_LEN+1],pack_transport[MYBMM_MODULE_NAME_LEN+1],pack_target[MYBMM_TARGET_LEN+1];
	sigset_t set;
	mybmm_config_t *conf;
	char *configfile,*logfile;

	battery_chem = battery_cap = -1;
	inv_type[0] = inv_transport[0] = inv_target[0] = 0;
	pack_type[0] = pack_transport[0] = pack_target[0] = 0;
	configfile = logfile = 0;
	while ((opt=getopt(argc, argv, "b:d:c:l:i:p:e:ht")) != -1) {
		switch (opt) {
		case 'b':
			battery_chem = atoi(strele(0,",",optarg));
			battery_cap = atoi(strele(1,",",optarg));
			break;
#ifdef DEBUG
		case 'd':
			debug=atoi(optarg);
			logopts |= LOG_DEBUG;
			break;
#endif
		case 'c':
			configfile=optarg;
			break;
		case 'l':
			logfile=optarg;
			break;
                case 'i':
			strncpy(inv_type,strele(0,":",optarg),sizeof(inv_type)-1);
			strncpy(inv_transport,strele(1,":",optarg),sizeof(inv_transport)-1);
			strncpy(inv_target,strele(2,":",optarg),sizeof(inv_target)-1);
			if (!strlen(inv_type) || !strlen(inv_transport) || !strlen(inv_target)) {
				printf("error: format is type:transport:target\n");
				usage(argv[0]);
				return 0;
			}
			break;
                case 'p':
			strncpy(pack_type,strele(0,":",optarg),sizeof(pack_type)-1);
			strncpy(pack_transport,strele(1,":",optarg),sizeof(pack_transport)-1);
			strncpy(pack_target,strele(2,":",optarg),sizeof(pack_target)-1);
			if (!strlen(pack_type) || !strlen(pack_transport) || !strlen(pack_target)) {
				printf("error: format is type:transport:target\n");
				usage(argv[0]);
				return 0;
			}
			break;
		case 'h':
		default:
			usage(argv[0]);
			exit(0);
                }
	}

	/* Open logfile if specified */
	if (logfile) logopts |= LOG_CREATE;
	log_open("mybmm",logfile,logopts);

	conf = get_config(configfile);
	dprintf(4,"conf: %p\n", conf);
	if (!conf) return 0;
#ifdef MQTT
	mqtt_init(conf);
#endif

	DDLOG("battery_chem: %d\n",battery_chem);
	if (battery_chem >= 0) {
		conf->battery_chem = battery_chem;
		conf->capacity = battery_cap >= 0 ? battery_cap : 100;
		conf->cell_low = conf->cell_crit_low = conf->cell_high = conf->cell_crit_high = -1;
		conf->c_rate = -1;
		battery_init(conf);
	}

	if (strlen(inv_type)) {
		mybmm_inverter_t *inv;
		inv = calloc(1,sizeof(*inv));
		if (!inv) {
			perror("calloc inverter");
			return 0;
		}
		strcpy(inv->type,inv_type);
		strcpy(inv->transport,inv_transport);
		strcpy(inv->target,inv_target);
		inverter_add(conf,inv);
	}

	if (strlen(pack_type)) {
		mybmm_pack_t *pack;
		pack = calloc(1,sizeof(*pack));
		if (!pack) {
			perror("calloc packerter");
			return 0;
		}
		strcpy(pack->type,pack_type);
		strcpy(pack->transport,pack_transport);
		strcpy(pack->target,pack_target);
		pack_add(conf,"pack",pack);
		pack_init(conf);
	}

	/* Ignore SIGPIPE */
        sigemptyset(&set);
        sigaddset(&set, SIGPIPE);
        sigprocmask(SIG_BLOCK, &set, NULL);

	/* USR causes config re-read (could also check file modification time) */
	reconf = 0;
        signal(SIGUSR1, usr1_handler);

	return conf;
}

int main(int argc, char **argv) {
	int i,startup,inv_reported,packs_reported,cell,in_range,npacks,repcap;
	mybmm_config_t *conf;
	mybmm_inverter_t *inv;
	mybmm_pack_t *pp;
	float cell_total, cell_min, cell_max, cell_diff, cell_avg;
	float min_cap,pack_voltage_total,pack_current_total,pack_capacity_total,pack_temp_total;
	uint32_t mask;
	time_t start,end,diff;

	/* Initialize system */
	conf = init(argc,argv);
	if (!conf) return 1;

	startup = 1;
	repcap = 0;

	charge_start(conf,1);

	while(1) {
		if (reconf) {
			dprintf(1,"calling reconfig...\n");
			reconfig(conf);
			reconf = 0;
		}
		inv_reported = packs_reported = 0;
		conf->battery_voltage = conf->battery_amps = 0.0;

		/* Get starting time */
		time(&start);

		/* Update inverter */
		inv = conf->inverter;
		if (inv) {
			inverter_read(inv);
			if (mybmm_check_state(inv,MYBMM_INVERTER_STATE_UPDATED)) inv_reported++;
		}

		in_range = 1;
		pack_update_all(conf,30);
		pack_voltage_total = pack_current_total = pack_capacity_total = pack_temp_total = 0.0;
		conf->pack_temp_max = -9999.99;
		conf->pack_cell_max = -9999.99;
		conf->pack_cell_min = 99999999.99;
		npacks = list_count(conf->packs);
		list_reset(conf->packs);
		while((pp = list_get_next(conf->packs)) != 0) {
			if (!mybmm_check_state(pp,MYBMM_PACK_STATE_UPDATED)) continue;
			dprintf(2,"pp->cells: %d\n", pp->cells);
			cell_min = conf->cell_crit_high;
			cell_max = 0.0;
			cell_total = 0;
			mask = 1;
			for(cell=0; cell < pp->cells; cell++) {
				dprintf(5,"pack %s cell[%d]: %.3f%s\n", pp->name, cell, pp->cellvolt[cell],
					(pp->balancebits & mask ? "*" : ""));
				dprintf(5,"cell_min: %.3f, cell_max: %.3f\n", cell_min, cell_max);
				if (pp->cellvolt[cell] < cell_min) cell_min = pp->cellvolt[cell];
				if (pp->cellvolt[cell] > cell_max) cell_max = pp->cellvolt[cell];
				cell_total += pp->cellvolt[cell];
				dprintf(5,"cell_crit_low: %.3f, cell_crit_high: %.3f\n",
					conf->cell_crit_low, conf->cell_crit_high);
				if (pp->cellvolt[cell] <= conf->cell_crit_low) {
					/* If any cell reaches critical low, immediate shutdown */
					dprintf(1,"*** CRIT LOW VOLT ***\n");
					mybmm_set_state(conf,MYBMM_CRIT_CELLVOLT);
//					mybmm_emergency_shutdown(conf);
//					do_shutdown(conf);
					in_range = 0;
					break;
				} else if (pp->cellvolt[cell] >= conf->cell_crit_high) {
					/* If any cell reaches crit high, consume it ASAP */
					mybmm_set_state(conf,MYBMM_CRIT_CELLVOLT);
					/* Disable charging */
					charge_stop(conf,1);
					/* Set SoC to 101% */
					mybmm_force_soc(conf,101);
					dprintf(1,"*** CRIT HIGH VOLT ***\n");
//					do_shutdown(conf);
					in_range = 0;
					break;
				}
				mask <<= 1;
			}
			if (!in_range) break;
			cell_diff = cell_max - cell_min;
			cell_avg = cell_total / conf->cells;
			dprintf(2,"cells: total: %.3f, min: %.3f, max: %.3f, diff: %.3f, avg: %.3f\n",
				cell_total, cell_min, cell_max, cell_diff, cell_avg);
			dprintf(2,"min_cap: %.3f, pp->capacity: %.3f\n", min_cap, pp->capacity);
			if (pp->capacity < min_cap) min_cap = pp->capacity;
			pack_voltage_total += cell_total;
			pack_current_total += pp->current;
			for(i=0; i < pp->ntemps; i++) {
				pack_temp_total += pp->temps[i];
				if (pp->temps[i] > conf->pack_temp_max) conf->pack_temp_max = pp->temps[i];
			}
			if (cell_min < conf->pack_cell_min) conf->pack_cell_min = cell_min;
			if (cell_max > conf->pack_cell_max) conf->pack_cell_max = cell_max;
			if (mybmm_check_state(pp,MYBMM_PACK_STATE_DISCHARGING)) pack_capacity_total += pp->capacity;
			packs_reported++;
		}
		/* Are we in a critvolt state and are all cells in range now? */
		dprintf(2,"in_range: %d\n", in_range);
		if (mybmm_is_critvolt(conf) && in_range) {
			mybmm_clear_state(conf,MYBMM_CRIT_CELLVOLT);
			if (mybmm_check_state(conf,MYBMM_FORCE_SOC)) mybmm_clear_state(conf,MYBMM_FORCE_SOC);
			/* Close contactors? */
		}
		if (npacks) dprintf(1,"%d/%d packs reported\n",packs_reported,npacks);

		/* If we dont have any inverts or packs, no sense continuing... */
		dprintf(1,"inv_reported: %d, pack_reported: %d\n", inv_reported, packs_reported);
		if (!inv_reported && !packs_reported) { if (!conf->filename && !conf->inverter) {
				printf("no config file, no inverter specified and no pack specified, nothing to do!\n");
				break;
			} else {
				sleep(conf->interval);
				continue;
			}
		}

		if (!(int)conf->capacity) {
			/* For capacity, if user capacity is set, use that.  */
			if (conf->user_capacity > 0.0) {
				conf->capacity = conf->user_capacity;
				repcap = 1;
			/* otherwise if we have packs and all packs have reported in, use total */
			} else if (npacks && packs_reported == npacks) {
				conf->capacity = pack_capacity_total;
				repcap = 1;
			}
		}
		if (repcap) {
			lprintf(0,"Capacity: %.1f\n", conf->capacity);
			conf->kwh = (conf->capacity * conf->system_voltage) / 1000.0;
			lprintf(0,"kWh: %.1f\n", conf->kwh);
			repcap = 0;
		}

		dprintf(1,"packs_reported: %d, pack_temp_total: %2.1f, inv_reported: %d, inv->battery_temp: %2.1f\n",
			packs_reported, pack_temp_total, inv_reported, inv->battery_temp);
		conf->battery_temp = packs_reported ? (pack_temp_total / packs_reported) : (inv_reported ? inv->battery_temp : 25.0);
		if (packs_reported && (conf->use_packv || !inv_reported)) {
			conf->battery_voltage = pack_voltage_total / packs_reported;
			conf->battery_amps = pack_current_total / packs_reported;
		} else if (inv_reported) {
			conf->battery_voltage = inv->battery_voltage;
			conf->battery_amps = inv->battery_amps;
		}
#if 0
		/* XXX SIM */
		if (!(int)conf->tvolt) conf->tvolt = conf->battery_voltage;
		if (mybmm_check_state(conf,MYBMM_CHARGING)) {
			if (conf->charge_mode == 1) conf->tvolt += 1;
			else if (conf->charge_mode == 2) conf->cv_start_time -= (30 * 60);
		} else {
			conf->tvolt -= 1;
		}
		conf->battery_voltage = conf->tvolt;
#endif

#if 0
		conf->charge_voltage = conf->user_charge_voltage < 0.0 ? conf->cell_high * conf->cells : conf->user_charge_voltage;

		dprintf(2,"user_discharge_voltage: %.1f, user_discharge_amps: %.1f\n", conf->user_discharge_voltage, conf->user_discharge_amps);
		conf->discharge_voltage = conf->user_discharge_voltage < 0.0 ? conf->cell_low * conf->cells : conf->user_discharge_voltage;
		dprintf(2,"conf->e_rate: %f, conf->capacity: %f\n", conf->e_rate, conf->capacity);
		conf->discharge_amps = conf->user_discharge_amps < 0.0 ? conf->e_rate * conf->capacity : conf->user_discharge_amps;
		lprintf(0,"Discharge voltage: %.1f, Discharge amps: %.1f\n", conf->discharge_voltage, conf->discharge_amps);

		dprintf(2,"user_charge_voltage: %.1f, user_charge_amps: %.1f\n", conf->user_charge_voltage, conf->user_charge_amps);
		conf->charge_voltage = conf->charge_target_voltage = conf->user_charge_voltage < 0.0 ? conf->cell_high * conf->cells : conf->user_charge_voltage;
		dprintf(2,"conf->c_rate: %f, conf->capacity: %f\n", conf->c_rate, conf->capacity);
		conf->charge_amps = conf->user_charge_amps < 0.0 ? conf->c_rate * conf->capacity : conf->user_charge_amps;
		lprintf(0,"Charge voltage: %.1f, Charge amps: %.1f\n", conf->charge_voltage, conf->charge_amps);

		lprintf(0,"Battery: voltage: %.1f, current: %.1f, temp: %.1f\n", conf->battery_voltage, conf->battery_amps, conf->battery_temp);
#endif

		conf->soc = conf->user_soc < 0.0 ? ( ( conf->battery_voltage - conf->discharge_voltage) / (conf->charge_target_voltage - conf->discharge_voltage) ) * 100.0 : conf->user_soc;
		lprintf(0,"SoC: %.1f\n", conf->soc);
		conf->soh = 100.0;

		charge_check(conf);

		/* Update inverter */
		dprintf(2,"startup: %d\n", startup);
		if (inv_reported) {
			if (startup == 1) {
				charge_stop(conf,0);
				conf->charge_amps = 0.1;
				conf->soc = 99.9;
			}
			inverter_write(conf->inverter);
		}
		if (startup) {
			startup++;
			if (startup == 3) {
				dprintf(1,"****** SOC: %.1f\n", conf->soc);
				if (conf->soc < 95.0) charge_start(conf,1);
				startup = 0;
			}
		}

		/* Get ending time */
		time(&end);

		dprintf(3,"start: %d, end: %d, diff: %d, interval: %d\n",(int)start,(int)end,(int)end-(int)start,conf->interval);
		diff = conf->interval - (end - start);
		if (end - start < conf->interval) {
			dprintf(1,"Sleeping for %d seconds...\n",(int)diff);
			sleep(diff);
		}
	}

	return 0;
}
