
#include "mybmm.h"
#include "battery.h"

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
		if (conf->d_rate < 0) conf->d_rate = 1;
		if (conf->cells < 0) conf->cells = 14;
		break;
	case BATTERY_CHEM_LIFEPO4:
		dprintf(1,"battery_chem: LIFEPO4\n");
		if (conf->cell_low < 0) conf->cell_low = 2.5;
		if (conf->cell_crit_low < 0) conf->cell_crit_low = 2.0;
		if (conf->cell_high < 0) conf->cell_high = 3.4;
		if (conf->cell_crit_high < 0) conf->cell_crit_high = 3.65;
		if (conf->c_rate < 0) conf->c_rate = 1;
		if (conf->d_rate < 0) conf->d_rate = 1;
		if (conf->cells < 0) conf->cells = 16;
		break;
	case BATTERY_CHEM_TITANATE:
		dprintf(1,"battery_chem: TITANATE\n");
		if (conf->cell_low < 0) conf->cell_low = 1.81;
		if (conf->cell_crit_low < 0) conf->cell_crit_low = 1.8;
		if (conf->cell_high < 0) conf->cell_high = 2.84;
		if (conf->cell_crit_high < 0) conf->cell_crit_high = 2.85;
		if (conf->c_rate < 0) conf->c_rate = 1;
		if (conf->d_rate < 0) conf->d_rate = 10;
		if (conf->cells < 0) conf->cells = 22;
		break;
	}
	dprintf(1,"cell_low: %.1f\n", conf->cell_low);
	dprintf(1,"cell_crit_low: %.1f\n", conf->cell_crit_low);
	dprintf(1,"cell_high: %.1f\n", conf->cell_high);
	dprintf(1,"cell_crit_high: %.1f\n", conf->cell_crit_high);
	dprintf(1,"c_rate: %.1f\n", conf->c_rate);

	return 0;
}
