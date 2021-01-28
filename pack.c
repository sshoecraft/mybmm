
#define _GNU_SOURCE
#include <sched.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include "mybmm.h"
#include "uuid.h"
#include "worker.h"
#include "parson.h"

#ifdef MQTT
#include "mqtt.h"
int pack_mqtt_send(mybmm_config_t *conf,mybmm_pack_t *pp) {
	register int i,j;
	char temp[256],*p;
	unsigned long mask;
	JSON_Value *root_value;
	JSON_Object *root_object;
	struct pack_states {
		int mask;
		char *label;
	} states[] = {
		{ MYBMM_PACK_STATE_CHARGING, "Charging" },
		{ MYBMM_PACK_STATE_DISCHARGING, "Discharging" },
		{ MYBMM_PACK_STATE_BALANCING, "Balancing" },
	};
#define NSTATES (sizeof(states)/sizeof(struct pack_states))

	/* Create JSON data */
	root_value = json_value_init_object();
	root_object = json_value_get_object(root_value);

	if (get_timestamp(temp,sizeof(temp),1) == 0) json_object_set_string(root_object, "timestamp", temp);
	json_object_set_string(root_object, "name", pp->name);
	json_object_set_string(root_object, "uuid", pp->uuid);
	json_object_set_number(root_object, "state", pp->state);
	json_object_set_number(root_object, "errcode", pp->error);
	json_object_set_string(root_object, "errmsg", pp->errmsg);
	json_object_set_number(root_object, "capacity", pp->capacity);
	json_object_set_number(root_object, "voltage", pp->voltage);
	json_object_set_number(root_object, "current", pp->current);
	if (pp->ntemps) {
		p = temp;
		p += sprintf(p,"[ ");
		dprintf(4,"ntemps: %d\n", pp->ntemps);
		for(i=0; i < pp->ntemps; i++) {
			if (i) p += sprintf(p,",");
			p += sprintf(p, "%.1f",pp->temps[i]);
		}
		strcat(temp," ]");
		dprintf(4,"temp: %s\n", temp);
		json_object_dotset_value(root_object, "temps", json_parse_string(temp));
	}
	if (pp->cells) {
		p = temp;
		p += sprintf(p,"[ ");
		for(i=0; i < pp->cells; i++) {
			if (i) p += sprintf(p,",");
			p += sprintf(p, "%.3f",pp->cellvolt[i]);
		}
		strcat(temp," ]");
		dprintf(4,"temp: %s\n", temp);
		json_object_dotset_value(root_object, "cellvolt", json_parse_string(temp));
#if 0
		p = temp;
		p += sprintf(p,"[ ");
		for(i=0; i < pp->cells; i++) {
			if (i) p += sprintf(p,",");
			p += sprintf(p, "%.3f",pp->cellres[i]);
		}
		strcat(temp," ]");
		dprintf(4,"temp: %s\n", temp);
		json_object_dotset_value(root_object, "cellres", json_parse_string(temp));
#endif
	}
	json_object_set_number(root_object, "cell_min", pp->cell_min);
	json_object_set_number(root_object, "cell_max", pp->cell_max);
	json_object_set_number(root_object, "cell_diff", pp->cell_diff);
	json_object_set_number(root_object, "cell_avg", pp->cell_avg);

	/* States */
	temp[0] = 0;
	p = temp;
	for(i=j=0; i < NSTATES; i++) {
		if (mybmm_check_state(pp,states[i].mask)) {
			if (j) p += sprintf(p,",");
			p += sprintf(p,states[i].label);
			j++;
		}
	}
	json_object_set_string(root_object, "states", temp);

	mask = 1;
	for(i=0; i < pp->cells; i++) {
		temp[i] = ((pp->balancebits & mask) != 0 ? '1' : '0');
		mask <<= 1;
	}
	temp[i] = 0;
	json_object_set_string(root_object, "balancebits", temp);

	/* TODO: protection info ... already covered with 'error/errmsg?' */
#if 0
        struct {
                unsigned sover: 1;              /* Single overvoltage protection */
                unsigned sunder: 1;             /* Single undervoltage protection */
                unsigned gover: 1;              /* Whole group overvoltage protection */
                unsigned gunder: 1;             /* Whole group undervoltage protection */
                unsigned chitemp: 1;            /* Charge over temperature protection */
                unsigned clowtemp: 1;           /* Charge low temperature protection */
                unsigned dhitemp: 1;            /* Discharge over temperature protection */
                unsigned dlowtemp: 1;           /* Discharge low temperature protection */
                unsigned cover: 1;              /* Charge overcurrent protection */
                unsigned cunder: 1;             /* Discharge overcurrent protection */
                unsigned shorted: 1;            /* Short circuit protection */
                unsigned ic: 1;                 /* Front detection IC error */
                unsigned mos: 1;                /* Software lock MOS */
        } protect;
#endif
//	p = json_serialize_to_string_pretty(root_value);
	p = json_serialize_to_string(root_value);
#if 0
	dprintf(1,"sending mqtt data...\n");
	if (mqtt_send(pp->mqtt_handle, p, 15)) {
		dprintf(1,"mqtt send error!\n");
		return 1;
	}
#else
	sprintf(temp,"%s/%s",conf->mqtt_topic,pp->name);
	mqtt_fullsend(conf->mqtt_broker,pp->name, p, temp);
#endif
	json_free_serialized_string(p);
	json_value_free(root_value);
	return 0;
}

void pack_mqtt_reconnect(void *ctx, char *cause) {
	mybmm_pack_t *pp = ctx;

	dprintf(1,"++++ RECONNECT: cause: %s\n", cause);
	mqtt_connect(pp->mqtt_handle,20);
}
#endif

//int do_pack_update(void *ctx) {
//	mybmm_pack_t *pp = ctx;
int do_pack_update(mybmm_pack_t *pp) {
	int r;

	dprintf(2,"pack: name: %s, type: %s, transport: %s\n", pp->name, pp->type, pp->transport);
	dprintf(4,"%s: opening...\n", pp->name);
	if (pp->open(pp->handle)) {
		dprintf(1,"%s: open error\n",pp->name);
		return 1;
	}
	dprintf(4,"%s: reading...\n", pp->name);
	r = pp->read(pp->handle,0,0);
	dprintf(4,"%s: closing\n", pp->name);
	pp->close(pp->handle);
	dprintf(4,"%s: returning: %d\n", pp->name, r);
	if (!r) {
		float total;
		int i;

		mybmm_set_state(pp,MYBMM_PACK_STATE_UPDATED);
		/* Set min/max/diff/avg */
		pp->cell_max = 0.0;
		pp->cell_min = pp->conf->cell_crit_high;
		total = 0.0;
		for(i=0; i < pp->cells; i++) {
			if (pp->cellvolt[i] < pp->cell_min)
				pp->cell_min = pp->cellvolt[i];
			if (pp->cellvolt[i] > pp->cell_max)
				pp->cell_max = pp->cellvolt[i];
			total += pp->cellvolt[i];
		}
		pp->cell_diff = pp->cell_max - pp->cell_min;
		pp->cell_avg = total / pp->cells;
		if (!(int)pp->voltage) pp->voltage = total;
	}
	return r;
}
int _do_pack_update(void *ctx) {
	exit(do_pack_update(ctx));
}

#define TIMEOUT 10
#define STACK_SIZE 32768

int pack_update(mybmm_pack_t *pp) {
	int pid,r,status;
	time_t start,curr,diff;
	char *stack, *stackTop;

	stack = mmap(NULL, STACK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
	if (stack == MAP_FAILED) {
		perror("mmap");
		return -1;
	}
	stackTop = stack + STACK_SIZE;

	dprintf(1,"cloning...\n");
//	pid = clone(_do_pack_update, stackTop, CLONE_FILES|CLONE_FS|CLONE_VM|SIGCHLD, pp);
	pid = clone(_do_pack_update, stackTop, CLONE_VM|SIGCHLD, pp);
	dprintf(1,"pid: %d\n", pid);
	if (pid < 0) {
		perror("clone");
		munmap(stack,STACK_SIZE);
		return -1;
	}
	sleep(1);
	dprintf(1,"waiting on pid...\n");
	time(&start);
	while(1) {
		r = waitpid(pid,&status,WNOHANG);
		dprintf(1,"r: %d\n",r);
		if (r < 0) {
			perror("waitpid");
			break;
		}
		dprintf(1,"WIFEXITED: %d\n", WIFEXITED(status));
		if (WIFEXITED(status)) break;
		time(&curr);
		diff = curr - start;
		dprintf(1,"start: %d, curr: %d, diff: %d\n", (int)start, (int)curr, (int)diff);
		if (diff >= TIMEOUT) {
			dprintf(1,"KILLING PID: %d\n", pid);
			kill(SIGTERM,pid);
//			kill(9,pid);
			break;
		}
		sleep(1);
	}
	if (WIFEXITED(status)) dprintf(1,"WEXITSTATUS: %d\n", WEXITSTATUS(status));
	status = (WIFEXITED(status) ? WEXITSTATUS(status) : 1);
	dprintf(1,"status: %d\n", status);
	if (!status) mybmm_set_state(pp,MYBMM_PACK_STATE_UPDATED);
	munmap(stack,STACK_SIZE);
	return status;

#if 0
	dprintf(1,"forking...\n");
	pid = fork();
	if (pid < 0) {
		/* Error */
		perror("fork");
		return -1;
	} else if (pid == 0) {
		/* Call func */
		_exit(do_pack_update(pp));
	} else {
		dprintf(1,"waiting on pid...\n");
		time(&start);
		while(1) {
			r = waitpid(pid,&status,WNOHANG);
			dprintf(1,"r: %d\n",r);
			if (r < 0) return 1;
			dprintf(1,"WIFEXITED: %d\n", WIFEXITED(status));
			if (WIFEXITED(status)) break;
			time(&curr);
			diff = curr - start;
			dprintf(1,"start: %d, curr: %d, diff: %d\n", (int)start, (int)curr, (int)diff);
			if (diff >= TIMEOUT) {
				dprintf(1,"KILLING PID: %d\n", pid);
				kill(SIGTERM,pid);
				break;
			}
			sleep(1);
		}
		if (WIFEXITED(status)) dprintf(1,"WEXITSTATUS: %d\n", WEXITSTATUS(status));
		status = (WIFEXITED(status) ? WEXITSTATUS(status) : 1);
		dprintf(1,"status: %d\n", status);
		if (!status) mybmm_set_state(pp,MYBMM_PACK_STATE_UPDATED);
		return status;
	}

	return 0;
#endif
}

int pack_update_all(mybmm_config_t *conf, int wait) {
	mybmm_pack_t *pp;

	/* main loop */
	conf->cell_crit_high = 9.9;
	dprintf(1,"updating...\n");
	list_reset(conf->packs);
	while((pp = list_get_next(conf->packs)) != 0) {
		mybmm_clear_state(pp,MYBMM_PACK_STATE_UPDATED);
		worker_exec(conf->pack_pool,(worker_func_t)do_pack_update,pp);
	}
	worker_wait(conf->pack_pool,wait);
	worker_killbusy(conf->pack_pool);

	/* Make sure all closed (in case update was killed) */
	list_reset(conf->packs);
	while((pp = list_get_next(conf->packs)) != 0) {
		pp->close(pp->handle);
		dprintf(1,"%s: updated: %d\n", pp->name, mybmm_check_state(pp,MYBMM_PACK_STATE_UPDATED));
#ifdef MQTT
		if (mybmm_check_state(pp,MYBMM_PACK_STATE_UPDATED)) pack_mqtt_send(conf,pp);
#endif
	}

	return 0;
}

int pack_add(mybmm_config_t *conf, char *packname, mybmm_pack_t *pp) {
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
	if (!strlen(pp->transport)) return 0;
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

#ifdef MQTT
	if (strlen(conf->mqtt_broker)) {
		char topic[192];
		int r;

		/* Create a new MQTT session and connect to the broker */
		sprintf(topic,"%s/%s",conf->mqtt_topic,pp->name);
		DDLOG("topic: %s\n",topic);
		pp->mqtt_handle = mqtt_new(conf->mqtt_broker,pp->name,topic);

		dprintf(4,"Connecting to Broker: %s\n",conf->mqtt_broker);
		if (mqtt_connect(pp->mqtt_handle,20)) return 1;

		/* Reconnect if lost */
		r = mqtt_setcb(pp->mqtt_handle, pp, pack_mqtt_reconnect, 0, 0);
		dprintf(1,"setcb rc: %d\n", r);
	}
#endif

	/* Get capability mask */
	dprintf(1,"capabilities: %02x\n", mp->capabilities);
	pp->capabilities = mp->capabilities;

	/* Set the convienience funcs */
	pp->open = mp->open;
	pp->read = mp->read;
	pp->close = mp->close;
	pp->control = mp->control;

	/* Add the pack */
	dprintf(3,"adding pack...\n");
	list_add(conf->packs,pp,0);
	pp->conf = conf;

	dprintf(3,"done!\n");
	return 0;
}

int pack_init(mybmm_config_t *conf) {
	mybmm_pack_t *pp;
	char name[32];
	int i,c,d;

	/* Read pack info */
	for(i=0; i < MYBMM_MAX_PACKS; i++) {
		/* Fill the section name in and read the config */
		sprintf(name,"pack_%02d",i+1);
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

	/* If no packs, return now */
	if (list_count(conf->packs) == 0) return 0;

	/* Create the pack worker pool */
	conf->pack_pool = worker_create_pool(list_count(conf->packs));
	if (!conf->pack_pool) return 1;

	/* Go through all packs and see if all have charge/discharge control */
	c = d = 1;
	list_reset(conf->packs);
	while((pp = list_get_next(conf->packs)) != 0) {
		if (!mybmm_check_cap(pp,MYBMM_CHARGE_CONTROL))
			c = 0;
		else if (!mybmm_check_cap(pp,MYBMM_DISCHARGE_CONTROL))
			d = 0;
	}
	if (c) {
		dprintf(0,"all packs have charge control!\n");
		mybmm_set_cap(conf,MYBMM_CHARGE_CONTROL);
	} else {
		dprintf(0,"warning: all packs DO NOT have charge control!\n");
	}
	if (d) {
		dprintf(0,"all packs have discharge control!\n");
		mybmm_set_cap(conf,MYBMM_DISCHARGE_CONTROL);
	} else {
		dprintf(0,"warning: all packs DO NOT have discharge control!\n");
	}
	dprintf(3,"done!\n");
	return 0;
}

#if 0
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
#endif
