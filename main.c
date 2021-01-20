
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
	int opt,inv_reported,packs_reported,cell,in_range,npacks;
	char type[MYBMM_MODULE_NAME_LEN+1],transport[MYBMM_MODULE_NAME_LEN+1],target[MYBMM_TARGET_LEN+1];
//	char *opts;
	mybmm_config_t *conf;
	mybmm_inverter_t *inv;
	mybmm_pack_t *pp;
	sigset_t set;
	float cell_total, cell_min, cell_max, cell_diff, cell_avg;
	float min_cap,pack_voltage_total,pack_current_total,pack_capacity_total;
	uint32_t mask;
	time_t start,end;
	char *configfile;

//	opts = 0;
	type[0] = transport[0] = target[0] = 0;
	configfile = 0;
	while ((opt=getopt(argc, argv, "d:c:p:e:h")) != -1) {
		switch (opt) {
		case 'd':
			debug=atoi(optarg);
			break;
		case 'c':
			configfile=optarg;
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
#if 0
		case 'e':
			opts = optarg;
			break;
#endif
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
	conf = get_config((configfile ? configfile : "mybmm.conf"));
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
		if (reconf) {
			dprintf(1,"calling reconfig...\n");
			reconfig(conf);
			reconf = 0;
		}
		inv_reported = packs_reported = 0;
		conf->battery_voltage = conf->battery_current = 0.0;

		/* Get starting time */
		time(&start);

		/* Update inverter */
		inv = conf->inverter;
		if (inv) {
			inverter_read(inv);
			if (mybmm_check_state(inv,MYBMM_INVERTER_STATE_UPDATED)) {
				inv_reported++;
				if ((int)inv->battery_voltage) conf->battery_voltage = inv->battery_voltage;
				if ((int)inv->battery_current) conf->battery_current = inv->battery_current;
			}
		}

		in_range = 1;
		pack_update_all(conf,15);
		pack_voltage_total = pack_current_total = pack_capacity_total = 0.0;
		npacks = list_count(conf->packs);
		list_reset(conf->packs);
		while((pp = list_get_next(conf->packs)) != 0) {
			if (!mybmm_check_state(pp,MYBMM_PACK_STATE_UPDATED)) continue;
			dprintf(1,"pp->cells: %d\n", pp->cells);
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
					mybmm_set_state(conf,MYBMM_CRIT_CELLVOLT);
//					mybmm_emergency_shutdown(conf);
					dprintf(1,"*** CRIT LOW VOLT ***\n");
//					do_shutdown(conf);
					in_range = 0;
					break;
				} else if (pp->cellvolt[cell] >= conf->cell_crit_high) {
					/* If any cell reaches crit high, consume it ASAP */
					mybmm_set_state(conf,MYBMM_CRIT_CELLVOLT);
					/* Disable charging */
					mybmm_disable_charging(conf);
					/* Set SoC to 100% */
					mybmm_force_soc(conf,100);
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
			dprintf(1,"cells: total: %.3f, min: %.3f, max: %.3f, diff: %.3f, avg: %.3f\n",
				cell_total, cell_min, cell_max, cell_diff, cell_avg);
			dprintf(2,"min_cap: %.3f, pp->capacity: %.3f\n", min_cap, pp->capacity);
			if (pp->capacity < min_cap) min_cap = pp->capacity;
			pack_voltage_total += cell_total;
			pack_voltage_total += pp->current;
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
		if (npacks) dprintf(0,"%d/%d packs reported\n",packs_reported,npacks);

		/* If we dont have any inverts or packs, no sense continuing... */
		dprintf(1,"inv_reported: %d, pack_reported: %d\n", inv_reported, packs_reported);
		if (!inv_reported && !packs_reported) {
			sleep(conf->interval);
			continue;
		}

		if (npacks) {
			display(conf);

			/* For capacity, if user capacity is set, use that.  */
			dprintf(2,"user_capacity: %2.2f, DC: %d, min_cap: %2.2f, pack_count: %d\n",conf->user_capacity,mybmm_check_cap(conf,MYBMM_DISCHARGE_CONTROL), min_cap, list_count(conf->packs));
			if (conf->user_capacity > 0.0) {
				conf->capacity = conf->user_capacity;
			/* otherwise check if all packs have discharge control, if so, sum up all packs */
			} else if (mybmm_check_cap(conf,MYBMM_DISCHARGE_CONTROL)) {
				conf->capacity = pack_capacity_total;
			/* otherwise, use the min of all packs */
			} else {
				conf->capacity = min_cap * list_count(conf->packs);
			}
			dprintf(0,"Capacity: %.1f\n", conf->capacity);
			conf->kwh = (conf->capacity * conf->system_voltage) / 1000.0;
			dprintf(0,"kWh: %.1f\n", conf->kwh);
		}

		if (npacks) {
			dprintf(2,"pack_voltage_total: %.1f, pack_current_total: %.1f, packs_reported: %d\n",
				pack_voltage_total, pack_current_total, packs_reported);
			if (!(int)conf->battery_voltage) conf->battery_voltage = pack_voltage_total / packs_reported;
			if (!(int)conf->battery_current) conf->battery_current = pack_current_total / packs_reported;
		}
		dprintf(0,"Battery voltage: %.1f, Battery current: %.1f\n", conf->battery_voltage, conf->battery_current);

		dprintf(2,"user_charge_voltage: %.1f, user_charge_amps: %.1f\n", conf->user_charge_voltage, conf->user_charge_amps);
		conf->charge_voltage = conf->user_charge_voltage > 0.0 ? conf->user_charge_voltage : conf->cell_high * conf->cells;
		conf->charge_amps = conf->user_charge_amps > 0.0 ? conf->user_charge_amps : ((conf->c_rate * conf->capacity) * 2) / 3;
		dprintf(0,"Charge voltage: %.1f, Charge amps: %.1f\n", conf->charge_voltage, conf->charge_amps);

		dprintf(2,"user_discharge_voltage: %.1f, user_discharge_amps: %.1f\n", conf->user_discharge_voltage, conf->user_discharge_amps);
		conf->discharge_voltage = conf->user_discharge_voltage > 0.0 ? conf->user_discharge_voltage : conf->cell_low * conf->cells;
		conf->discharge_amps = conf->user_discharge_amps > 0.0 ? conf->user_discharge_amps : conf->c_rate * conf->capacity;
		dprintf(0,"Discharge voltage: %.1f, Discharge amps: %.1f\n", conf->discharge_voltage, conf->discharge_amps);

		conf->soc = conf->user_soc >= 0.0 ? conf->user_soc : ( ( conf->battery_voltage - conf->discharge_voltage) / (conf->charge_voltage - conf->discharge_voltage) ) * 100.0;
		dprintf(0,"SoC: %.1f\n", conf->soc);
		conf->soh = 100.0;

		/* Update inverter */
		if (inv_reported) inverter_write(conf->inverter);

		/* Get ending time */
		time(&end);

		dprintf(3,"start: %d, end: %d, diff: %d, interval: %d\n",(int)start,(int)end,(int)end-(int)start,conf->interval);
		opt = conf->interval - (end - start);
		if (end - start < conf->interval) {
			dprintf(1,"Sleeping for %d seconds...\n",opt);
			sleep(opt);
		}
	}

	return 0;
}
