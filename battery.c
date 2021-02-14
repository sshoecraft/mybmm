
#include <time.h>
#include "mybmm.h"
#include "battery.h"

#define HOURS(n) (n * 3600)

int battery_init(mybmm_config_t *conf) {
	/* These values are probably wrong and I'll need to revisit this */
	switch(conf->battery_chem) {
	default:
	case BATTERY_CHEM_LITHIUM:
		dprintf(1,"battery_chem: LITHIUM\n");
		if (conf->cell_low < 0) conf->cell_low = 3.0;
		if (conf->cell_crit_low < 0) conf->cell_crit_low = 2.95;
		if (conf->cell_high < 0) conf->cell_high = 4.2;
		if (conf->cell_crit_high < 0) conf->cell_crit_high = 4.25;
		if (conf->c_rate < 0) conf->c_rate = .7;
		if (conf->e_rate < 0) conf->e_rate = 1;
		if (conf->cells < 0) conf->cells = 14;
		conf->cell_min = 2.8;
		conf->cell_max = 4.2;
		break;
	case BATTERY_CHEM_LIFEPO4:
		dprintf(1,"battery_chem: LIFEPO4\n");
		if (conf->cell_low < 0) conf->cell_low = 2.5;
		if (conf->cell_crit_low < 0) conf->cell_crit_low = 2.0;
		if (conf->cell_high < 0) conf->cell_high = 3.4;
		if (conf->cell_crit_high < 0) conf->cell_crit_high = 3.70;
		if (conf->c_rate < 0) conf->c_rate = 1;
		if (conf->e_rate < 0) conf->e_rate = 1;
		if (conf->cells < 0) conf->cells = 16;
		conf->cell_min = 2.5;
		conf->cell_max = 3.65;
		break;
	case BATTERY_CHEM_TITANATE:
		dprintf(1,"battery_chem: TITANATE\n");
		if (conf->cell_low < 0) conf->cell_low = 1.81;
		if (conf->cell_crit_low < 0) conf->cell_crit_low = 1.8;
		if (conf->cell_high < 0) conf->cell_high = 2.85;
		if (conf->cell_crit_high < 0) conf->cell_crit_high = 2.88;
		if (conf->c_rate < 0) conf->c_rate = 1;
		if (conf->e_rate < 0) conf->e_rate = 10;
		if (conf->cells < 0) conf->cells = 22;
		conf->cell_min = 1.8;
		conf->cell_max = 2.85;
		break;
	}
//	conf->discharge_min_voltage = conf->user_discharge_voltage < 0.0 ? conf->cell_min * conf->cells : conf->user_discharge_max_voltage;
	conf->charge_max_voltage = conf->user_charge_voltage < 0.0 ? conf->cell_max * conf->cells : conf->user_charge_max_voltage;
	dprintf(1,"charge_max_voltage: %.1f\n", conf->charge_max_voltage);
	conf->charge_target_voltage = conf->cell_high * conf->cells;
	dprintf(1,"charge_target_voltage: %.1f\n", conf->charge_target_voltage);
	dprintf(1,"cell_low: %.1f\n", conf->cell_low);
	dprintf(1,"cell_crit_low: %.1f\n", conf->cell_crit_low);
	dprintf(1,"cell_high: %.1f\n", conf->cell_high);
	dprintf(1,"cell_crit_high: %.1f\n", conf->cell_crit_high);
	dprintf(1,"c_rate: %.1f\n", conf->c_rate);
	dprintf(1,"e_rate: %.1f\n", conf->e_rate);
	/* Default capacity is 0 if none specified (cant risk) */
	conf->capacity = conf->user_capacity < 0.0 ? 0 : conf->user_capacity;
	charge_start(conf,0);
	charge_stop(conf,0);
	return 0;
}

void charge_stop(mybmm_config_t *conf, int rep) {
	if (rep) lprintf(0,"*** ENDING CHARGE ***\n");
	mybmm_clear_state(conf,MYBMM_CHARGING);
	conf->charge_amps = 0.1;
	conf->charge_voltage = conf->user_charge_voltage < 0.0 ? conf->cell_high * conf->cells : conf->user_charge_voltage;

	dprintf(2,"user_discharge_voltage: %.1f, user_discharge_amps: %.1f\n", conf->user_discharge_voltage, conf->user_discharge_amps);
	conf->discharge_voltage = conf->user_discharge_voltage < 0.0 ? conf->cell_low * conf->cells : conf->user_discharge_voltage;
	dprintf(2,"conf->e_rate: %f, conf->capacity: %f\n", conf->e_rate, conf->capacity);
	conf->discharge_amps = conf->user_discharge_amps < 0.0 ? conf->e_rate * conf->capacity : conf->user_discharge_amps;
	if (rep) lprintf(0,"Discharge voltage: %.1f, Discharge amps: %.1f\n", conf->discharge_voltage, conf->discharge_amps);
}

void charge_start(mybmm_config_t *conf, int rep) {
	/* Do not start the charge until we have a capacity */
	if (!(int)conf->capacity) return;
	if (rep) lprintf(0,"*** STARTING CC CHARGE ***\n");
	mybmm_set_state(conf,MYBMM_CHARGING);
	dprintf(2,"user_charge_voltage: %.1f, user_charge_amps: %.1f\n", conf->user_charge_voltage, conf->user_charge_amps);
	conf->charge_voltage = conf->charge_target_voltage = conf->user_charge_voltage < 0.0 ? conf->cell_high * conf->cells : conf->user_charge_voltage;
	dprintf(2,"conf->c_rate: %f, conf->capacity: %f\n", conf->c_rate, conf->capacity);
	conf->charge_amps = conf->user_charge_amps < 0.0 ? conf->c_rate * conf->capacity : conf->user_charge_amps;
	if (rep) lprintf(0,"Charge voltage: %.1f, Charge amps: %.1f\n", conf->charge_voltage, conf->charge_amps);
	conf->charge_mode = 1;
	conf->tvolt = conf->battery_voltage;
	conf->start_temp = conf->battery_temp;
}

static void cvremain(time_t start, time_t end) {
	int diff,hours,mins;

	diff = (int)difftime(end,start);
	dprintf(1,"start: %ld, end: %ld, diff: %d\n", start, end, diff);
	if (diff > 0) {
		hours = diff / 3600;
		if (hours) diff -= (hours * 3600);
		dprintf(1,"hours: %d, diff: %d\n", hours, diff);
		mins = diff / 60;
		if (mins) diff -= (mins * 60);
		dprintf(1,"mins: %d, diff: %d\n", mins, diff);
	} else {
		hours = mins = diff = 0;
	}
	lprintf(0,"CV Time remaining: %02d:%02d:%02d\n",hours,mins,diff);
}

void charge_check(mybmm_config_t *conf) {
	time_t current_time;

	time(&current_time);
	if (mybmm_check_state(conf,MYBMM_CHARGING)) {
		/* If battery temp is <= 0, stop charging immediately */
		if (conf->battery_temp <= 0.0) {
			charge_stop(conf,1);
			return;
		}
		/* If battery temp <= 5C, reduce charge rate by 1/4 */
		if (conf->battery_temp <= 5.0) conf->charge_amps /= 4.0;

		/* Watch for rise in battery temp, anything above 5 deg C is an error */
		/* We could lower charge amps until temp goes down and then set that as max amps */
		if (pct(conf->battery_temp,conf->start_temp) > 5) {
			dprintf(0,"ERROR: current_temp: %.1f, start_temp: %.1f\n", conf->battery_temp, conf->start_temp);
			charge_stop(conf,1);
			return;
		}

		/* CC */
		if (conf->charge_mode == 1) {
			dprintf(1,"battery_amps: %.1f, charge_amps: %.1f, battery_voltage: %.1f, charge_target_voltage: %.1f, charge_voltage: %.1f, charge_max_voltage: %.1f\n", conf->battery_amps,conf->charge_amps,conf->battery_voltage,conf->charge_target_voltage,conf->charge_voltage,conf->charge_max_voltage);

			if (conf->battery_voltage > conf->charge_target_voltage || conf->pack_cell_max >= conf->cell_max) {
				time(&conf->cv_start_time);
				conf->charge_mode = 2;
				conf->charge_voltage = conf->user_charge_voltage < 0.0 ? conf->cell_high * conf->cells : conf->user_charge_voltage;
				lprintf(0,"*** STARTING CV CHARGE ***\n");
			} else if (conf->battery_amps < conf->charge_amps && conf->charge_voltage < conf->charge_max_voltage) {
				conf->charge_voltage += 0.1;
			}

		/* CV */
		} else if (conf->charge_mode == 2) {
			time_t end_time = conf->cv_start_time + HOURS(2);

			/* End saturation mode after 2 hours */
			cvremain(current_time,end_time);
			if (current_time >= end_time) {
				charge_stop(conf,1);
			}
		}
	} else {
		/* Discharging */
		dprintf(1,"battery_voltage: %.1f, discharge_voltage: %.1f, pack_cell_min: %.3f, cell_low: %.3f\n",
			conf->battery_voltage, conf->discharge_voltage, conf->pack_cell_min, conf->cell_low);
		if (conf->battery_voltage < conf->discharge_voltage || conf->pack_cell_min <= conf->cell_min) {
			/* Start charging */
			charge_start(conf,1);
		}
	}
}
