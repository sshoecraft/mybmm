
#include "mybmm.h"

void display(struct mybmm_config *conf) {
#if 0
	float *temps,*values,*summ,*bsum;
	float *fptr;
#else
#define NSUMM 4
#define NBSUM 2
	float temps[MYBMM_MAX_TEMPS][MYBMM_MAX_PACKS];
	float values[MYBMM_MAX_CELLS][MYBMM_MAX_PACKS];
	float summ[NSUMM][MYBMM_MAX_PACKS];
	float bsum[NBSUM][MYBMM_MAX_PACKS];
#endif
	float cell_total, cell_min, cell_max, cell_diff, cell_avg, min_cap;
	int x,y,cells,max_temps,pack_reported;
	char str[32],*p;
	char *slabels[NSUMM] = { "Min","Max","Avg","Diff" };
	char *tlabels[NBSUM] = { "Curr","Volt" };
	mybmm_inverter_t *inv;
	mybmm_pack_t *pp;

	cells = 0;
	max_temps = 0;
	list_reset(conf->packs);
	while((pp = list_get_next(conf->packs)) != 0) {
		if (!mybmm_check_state(pp,MYBMM_PACK_STATE_UPDATED)) continue;
		if (pp->ntemps > max_temps) max_temps = pp->ntemps;
		if (!cells) cells = pp->cells;
	}
	if (!cells) {
		dprintf(1,"error: no cells to display!\n");
		return;
	}
	dprintf(1,"cells: %d\n",cells);
#if 0
	values = calloc(list_count(conf->packs)*(cells*sizeof(float)),1);
	summ = calloc(list_count(conf->packs)*(nsumm*sizeof(float)),1);
	bsum = calloc(list_count(conf->packs)*(nbsum*sizeof(float)),1);
	dprintf(1,"temps: %p, values: %p, summ: %p, bsum: %p\n", temps,values,summ,bsum);
#else
	memset(temps,0,sizeof(temps));
	memset(values,0,sizeof(values));
	memset(summ,0,sizeof(summ));
	memset(bsum,0,sizeof(bsum));
#endif
	x = 0;
	pack_reported = 0;
	list_reset(conf->packs);
	while((pp = list_get_next(conf->packs)) != 0) {
		if (!mybmm_check_state(pp,MYBMM_PACK_STATE_UPDATED)) {
			cell_min = cell_max = cell_avg = cell_total = 0;
		} else {
			cell_min = conf->cell_crit_high;
			cell_max = 0.0;
			cell_total = 0;
			for(y=0; y < pp->cells; y++) {
				if (pp->cellvolt[y] < cell_min) cell_min = pp->cellvolt[y];
				if (pp->cellvolt[y] > cell_max) cell_max = pp->cellvolt[y];
#if 0
				fptr = values + ((x*cells) + y);
				*fptr = pp->cellvolt[y];
//				dprintf(1,"values[%d][%d]: %.3f\n", x, y, pp->cellvolt[y]);
#else
				values[y][x] = pp->cellvolt[y];
//				dprintf(1,"values[%d][%d]: %.3f\n", y, x, values[y][x]);
#endif
				cell_total += pp->cellvolt[y];
			}
			dprintf(2,"min_cap: %.3f, pp->capacity: %.3f\n", min_cap, pp->capacity);
			if (pp->capacity < min_cap) min_cap = pp->capacity;
			pack_reported++;
#if 0
			fptr = temps + (x*max_temps);
			for(y=0; y < pp->ntemps; y++) *fptr++ = pp->temps[y];
#endif
			for(y=0; y < pp->ntemps; y++) temps[y][x] = pp->temps[y];
			cell_avg = cell_total / pp->cells;
		}
		cell_diff = cell_max - cell_min;
		dprintf(1,"conf->cells: %d\n", conf->cells);
		dprintf(1,"cells: total: %.3f, min: %.3f, max: %.3f, diff: %.3f, avg: %.3f\n",
			cell_total, cell_min, cell_max, cell_diff, cell_avg);
#if 0
		fptr = summ + (x*nsumm);
		dprintf(1,"x: %d, fptr: %p, summ: %p\n", x, fptr, summ);
		*fptr++ = cell_min;
		*fptr++ = cell_max;
		*fptr++ = cell_avg;
		*fptr++ = cell_diff;
		fptr = bsum + (x*nbsum);
		*fptr++ = pp->current;
		*fptr++ = cell_total;
#else
		summ[0][x] = cell_min;
		summ[1][x] = cell_max;
		summ[2][x] = cell_avg;
		summ[3][x] = cell_diff;
		bsum[0][x] = pp->current;
		bsum[1][x] = cell_total;
#endif
		x++;
	}

	if (!debug) system("clear; echo \"**** $(date) ****\"");

	x = 0;
	inv = conf->inverter;
	if (inv) {
		dprintf(1,"inv: name: %s, type: %s\n",inv->name,inv->type);
		printf("\nInverter: %s\n",inv->name);
		if (mybmm_check_state(inv,MYBMM_INVERTER_STATE_UPDATED)) {
			if (mybmm_check_state(inv,MYBMM_INVERTER_STATE_RUNNING))
				p = "Running";
			else
				p = "Off";
		} else {
			p = "Error";
		}
		printf("State: %s\n", p);
		if (strcmp(p,"Error") != 0) {
			printf("Charge voltage: %.1f, Discharge voltage: %.1f\n", conf->charge_voltage, conf->discharge_voltage);
		}
//	} else {
//		dprintf(0,"\nInverter: None\n");
	}
#if 0
	conf->capacity = (conf->user_capacity == -1 ? min_cap * pack_reported++ : conf->user_capacity);
	printf("Capacity: %.1f\n", conf->capacity);
	conf->kwh = (conf->capacity * conf->system_voltage) / 1000.0;
	printf("kWh: %.1f\n", conf->kwh);
	printf("\n");
#endif

#define CFMT "%-5.5s  "
	/* Header */
	printf(CFMT,"");
	x = 1;
	list_reset(conf->packs);
	while((pp = list_get_next(conf->packs)) != 0) {
		sprintf(str," P%02d",x++);
		printf(CFMT,str);
	}
	printf("\n");
	/* Lines */
	printf(CFMT,"");
	list_reset(conf->packs);
	while((pp = list_get_next(conf->packs)) != 0) printf(CFMT,"----------------------");
	printf("\n");

#define FTEMP(v) ( ( ( (float)(v) * 9.0 ) / 5.0) + 32.0)
	/* Temps */
	for(y=0; y < max_temps; y++) {
		sprintf(str,"T%d",y+1);
		printf(CFMT,str);
		for(x=0; x < list_count(conf->packs); x++) {
#if 0
			fptr = temps + (x*2);
			sprintf(str,"%.3f",FTEMP(fptr[y]));
#else
//			sprintf(str,"%.1f",FTEMP(temps[y][x]));
			sprintf(str,"%.1f",temps[y][x]);
#endif
			printf(CFMT,str);
		}
		printf("\n");
	}
	if (max_temps) printf("\n");

	/* Cell values */
	for(y=0; y < cells; y++) {
		sprintf(str,"C%02d",y+1);
		printf(CFMT,str);
		for(x=0; x < list_count(conf->packs); x++) {
#if 0
			fptr = values + ((x*cells) + y);
			sprintf(str,"%.3f",*fptr);
#else
			sprintf(str,"%.3f",values[y][x]);
#endif
			printf(CFMT,str);
		}
		printf("\n");
	}
	printf("\n");

	/* Summ values */
	for(y=0; y < NSUMM; y++) {
		printf(CFMT,slabels[y]);
		for(x=0; x < list_count(conf->packs); x++) {
#if 0
			fptr = summ + (x*NSUMM);
			sprintf(str,"%.3f",fptr[y]);
#else
			sprintf(str,"%.3f",summ[y][x]);
#endif
			printf(CFMT,str);
		}
		printf("\n");
	}
	printf("\n");

	/* Current/Total */
	for(y=0; y < NBSUM; y++) {
		printf(CFMT,tlabels[y]);
		for(x=0; x < list_count(conf->packs); x++) {
#if 0
			fptr = bsum + (x*NBSUM);
			sprintf(str,"%.3f",fptr[y]);
#else
			sprintf(str,"%.3f",bsum[y][x]);
#endif
			printf(CFMT,str);
		}
		printf("\n");
	}

#if 0
	free(temps);
	free(values;
	free(summ;
	free(bsum;
#endif
}
