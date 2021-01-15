
#include <sys/signal.h>
#include "mybmm.h"

int debug = 1;

volatile uint8_t reconf;

void usr1_handler(int signo) {
	if (signo == SIGUSR1) {
		printf("reconf!\n");
		reconf = 1;
	}
}

extern int display(mybmm_config_t *conf);

extern void si_shutdown(void *);
void do_shutdown(mybmm_config_t *conf) {
	si_shutdown(conf->inverter->handle);
	exit(1);
}

int main(void) {
	struct mybmm_config *conf;
	mybmm_pack_t *pp;
	int inv_reported,pack_reported,cell,in_range;
	float cell_total, cell_min, cell_max, cell_diff, cell_avg;
	float min_cap,pack_voltage_total;
	uint32_t mask;
	sigset_t set;

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
	sleep(5);

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

	if (conf->inverter) inverter_start_update(conf);

	dprintf(1,"Starting main loop...\n");
	reconf = 0;
	while(1) {
		if (reconf) {
			dprintf(1,"calling reconfig...\n");
			reconfig(conf);
			reconf = 0;
		}

		/* Update inverter */
//		inverter_read(conf);
		if (conf->inverter) inv_reported = mybmm_check_state(conf->inverter,MYBMM_INVERTER_STATE_UPDATED);

#if 0
		in_range = 1;
		pack_update_all(conf);
		pack_voltage_total = 0.0;
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
			pack_reported++;
		}
		/* Are we in a critvolt state and are all cells in range now? */
		dprintf(1,"in_range: %d\n", in_range);
		if (mybmm_is_critvolt(conf) && in_range) {
			mybmm_clear_state(conf,MYBMM_CRIT_CELLVOLT);
			if (mybmm_check_state(conf,MYBMM_FORCE_SOC)) mybmm_clear_state(conf,MYBMM_FORCE_SOC);
			/* Close contactors? */
		}
		dprintf(0,"%d/%d packs reported\n",pack_reported,list_count(conf->packs));

		dprintf(1,"pack_voltage_total: %f, pack_reported: %d\n", pack_voltage_total, pack_reported);
		if (!(int)conf->battery_voltage) conf->battery_voltage = pack_voltage_total / pack_reported;
//		pack_current_average = pack_current_total / packs;
//		if (!(int)conf->battery_current) conf->battery_current = pack_current_average;

		/* If we dont have any inverts or packs, no sense continuing... */
		dprintf(1,"inv_reported: %d, pack_reported: %d\n", inv_reported, pack_reported);
		if (!inv_reported && !pack_reported) {
			sleep(conf->interval);
			continue;
		}
#endif

		display(conf);

		conf->capacity = (conf->user_capacity == 0 ? min_cap * pack_reported++ : conf->user_capacity);
		dprintf(0,"Capacity: %.1f\n", conf->capacity);
		conf->kwh = (conf->capacity * conf->system_voltage) / 1000.0;
		dprintf(0,"kWh: %.1f\n", conf->kwh);

		conf->soh = 100.0;

		/* Charge/Discharge amps */
//		conf->charge_amps = ((conf->c_rate * conf->capacity) * 2) / 3;
		conf->charge_amps = 12;
//		conf->discharge_amps = conf->c_rate * conf->capacity;
		conf->discharge_amps = 30.0;
		dprintf(0,"Charge amps: %.1f, Discharge amps: %.1f\n", conf->charge_amps, conf->discharge_amps);

		dprintf(1,"interval: %d\n",conf->interval);
//		sleep(conf->interval);
		sleep(30);
		printf("back\n");
	}

	return 0;
}
