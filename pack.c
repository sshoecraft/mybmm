
#include "mybmm.h"
#include "uuid.h"
#include "worker.h"

static int pack_update(mybmm_pack_t *pp) {
	int r;

//	dprintf(3,"pack: name: %s, type: %s, transport: %s\n", pp->name, pp->type, pp->transport);
	dprintf(5,"%s: opening...\n", pp->name);
	if (pp->open(pp->handle)) {
		dprintf(1,"%s: open error\n",pp->name);
		return 1;
	}
	dprintf(5,"%s: reading...\n", pp->name);
	r = pp->read(pp->handle,0,0);
	dprintf(5,"%s: closing\n", pp->name);
	pp->close(pp->handle);
	dprintf(5,"%s: returning: %d\n", pp->name, r);
	if (!r) mybmm_set_state(pp,MYBMM_PACK_STATE_UPDATED);
	return r;
}

static void *pack_thread(void *arg) {
	mybmm_config_t *conf = arg;
	mybmm_pack_t *pp;
	worker_pool_t *pool;

	pool = worker_create_pool(list_count(conf->packs));

	while(1) {
		list_reset(conf->packs);
		while((pp=list_get_next(conf->packs)) != 0) {
			mybmm_clear_state(pp,MYBMM_PACK_STATE_UPDATED);
			worker_exec(pool,(worker_func_t)pack_update,pp);
		}
		worker_wait(pool,0);

		while((pp=list_get_next(conf->packs)) != 0) {
			if (!mybmm_check_state(pp,MYBMM_PACK_STATE_UPDATED)) {
				dprintf(1,"pack %s update failed!\n",pp->name);
				pp->failed++;
			}
		}
//		sleep(conf->pack_refresh_interval);
		sleep(30);
	}

	worker_destroy_pool(pool,-1);
	return 0;
}

static int pack_add(mybmm_config_t *conf, char *packname, mybmm_pack_t *pp) {
        struct cfg_proctab packtab[] = {
		{ packname, "name", "Pack name", DATA_TYPE_STRING,&pp->name,sizeof(pp->name), 0 },
		{ packname, "uuid", "Pack UUID", DATA_TYPE_STRING,&pp->uuid,sizeof(pp->uuid), 0 },
		{ packname, "type", "Pack BMS type", DATA_TYPE_STRING,&pp->type,sizeof(pp->type), 0 },
		{ packname, "transport", "Pack transport", DATA_TYPE_STRING,&pp->transport,sizeof(pp->transport), 0 },
		{ packname, "target", "Pack address/interface/device", DATA_TYPE_STRING,&pp->target,sizeof(pp->target), 0 },
		{ packname, "opts", "Pack-specific options", DATA_TYPE_STRING,&pp->opts,sizeof(pp->opts), 0 },
		{ packname, "capacity", "Pack Capacity in AH", DATA_TYPE_FLOAT,&pp->capacity, 0, 0 },
		CFG_PROCTAB_END

	};
	mybmm_module_t *mp, *tp;

	cfg_get_tab(conf->cfg,packtab);
	if (!strlen(pp->name)) strcpy(pp->name,packname);
	if (debug >= 3) cfg_disp_tab(packtab,0,1);

	/* if we dont have a UUID, gen one */
	if (!strlen(pp->uuid)) {
		uint8_t uuid[16];

		dprintf(4,"gen'ing UUID...\n");
		uuid_generate_random(uuid);
		uuid_unparse(uuid, pp->uuid);
		dprintf(4,"pp->uuid: %s\n", pp->uuid);
		cfg_set_item(conf->cfg,packname,"uuid",0,pp->uuid);
		/* Signal conf to save the file */
		mybmm_set_state(conf,MYBMM_CONFIG_DIRTY);
	}

	/* Load the transport */
	tp = mybmm_load_module(conf,pp->transport,MYBMM_MODTYPE_TRANSPORT);
	if (!tp) {
		fprintf(stderr,"unable to load TRANSPORT module %s for %s, aborting\n",pp->transport,packname);
		return 1;
	}

	/* Load our module */
	mp = mybmm_load_module(conf,pp->type,MYBMM_MODTYPE_CELLMON);
	if (!mp) {
		fprintf(stderr,"unable to load CELLMON module %s for %s, aborting\n",pp->type,packname);
		return 1;
	}

	/* Create an instance of the bms */
	dprintf(3,"mp: %p\n",mp);
	if (mp) {
		pp->handle = mp->new(conf, pp, tp);
		if (!pp->handle) {
			fprintf(stderr,"module %s->new returned null!\n", mp->name);
			return 1;
		}
	}

	/* Set the convienience funcs */
	pp->open = mp->open;
	pp->read = mp->read;
	pp->close = mp->close;
	pp->control = mp->control;

	/* Add the pack */
	dprintf(3,"adding pack...\n");
	list_add(conf->packs,pp,0);

	dprintf(3,"done!\n");
	return 0;
}

int pack_init(mybmm_config_t *conf) {
	mybmm_pack_t *pp;
	char name[32];
	int i;
	pthread_attr_t attr;

	/* Read pack info */
	for(i=1; i < MYBMM_MAX_PACKS; i++) {
		/* Fill the section name in and read the config */
		sprintf(name,"pack_%02d",i);
		dprintf(3,"name: %s\n", name);

		if (!cfg_get_item(conf->cfg,name,"type")) break;
		pp = calloc(1,sizeof(*pp));
		if (!pp) {
			perror("calloc pack\n");
			return 1;
		}
		if (pack_add(conf,name,pp)) {
			free(pp);
			return 1;
		}

	}

	/* Create a detached thread */
	dprintf(3,"Creating thread...\n");
	if (pthread_attr_init(&attr)) {
		dprintf(0,"pthread_attr_init: %s\n",strerror(errno));
		return 1;
	}
	if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) {
		dprintf(0,"pthread_attr_setdetachstate: %s\n",strerror(errno));
		return 1;
	}
	if (pthread_create(&conf->pack_tid,&attr,&pack_thread,conf)) {
		dprintf(0,"pthread_create: %s\n",strerror(errno));
		return 1;
	}

	dprintf(3,"done!\n");
	return 0;
}

int pack_check(mybmm_config_t *conf,mybmm_pack_t *pp) {
	int cell,in_range;
	float cell_total, cell_min, cell_max, cell_diff, cell_avg;

	dprintf(1,"pp->cells: %d\n", pp->cells);
	cell_min = conf->cell_crit_high;
	cell_max = 0.0;
	cell_total = 0;
	in_range = 1;
	for(cell=0; cell < pp->cells; cell++) {
		dprintf(1,"pack %s cell[%d]: %.3f\n", pp->name, cell, pp->cellvolt[cell]);
		dprintf(1,"cell_min: %.3f, cell_max: %.3f\n", cell_min, cell_max);
		if (pp->cellvolt[cell] < cell_min) cell_min = pp->cellvolt[cell];
		if (pp->cellvolt[cell] > cell_max) cell_max = pp->cellvolt[cell];
		cell_total += pp->cellvolt[cell];
		dprintf(1,"cell_crit_low: %.3f, cell_crit_high: %.3f\n",
		conf->cell_crit_low, conf->cell_crit_high);
		if (pp->cellvolt[cell] <= conf->cell_crit_low) {
			/* If any cell reaches critical low, immediate shutdown */
			mybmm_set_state(conf,MYBMM_CRIT_CELLVOLT);
//			mybmm_emergency_shutdown(conf);
			dprintf(1,"*** CRIT LOW VOLT ***\n");
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
			in_range = 0;
			break;
		}
		if (!in_range) break;
		cell_diff = cell_max - cell_min;
		cell_avg = cell_total / conf->cells;
		dprintf(1,"cells: total: %.3f, min: %.3f, max: %.3f, diff: %.3f, avg: %.3f\n",
			cell_total, cell_min, cell_max, cell_diff, cell_avg);
	}
#if 0
	/* Are we in a critvolt state and are all cells in range now? */
	dprintf(1,"in_range: %d\n", in_range);
	if (mybmm_is_critvolt(conf) && in_range) {
		mybmm_clear_state(conf,MYBMM_CRIT_CELLVOLT);
		if (mybmm_check_state(conf,MYBMM_FORCE_SOC)) mybmm_clear_state(conf,MYBMM_FORCE_SOC);
		/* Close contactors? */
	}
#endif
	return (in_range ? 0 : 1);
}