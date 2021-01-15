
#define _GNU_SOURCE
#include <pthread.h>
#include <linux/can.h>
#include <sys/signal.h>
#include "mybmm.h"

struct preh_info {
	int cmus[15],ncmu;
	mybmm_pack_t *pp;
	pthread_t th;
	volatile uint32_t bitmaps[8];
	uint8_t messages[256][8];
	mybmm_module_t *tp;
	void *tp_handle;
	int open,stop;
	uint8_t *(*get_data)(struct preh_info *info, int id);
};
typedef struct preh_info preh_info_t;

#define _getshort(p) ((short) (*((p)) | ( (*((p)+1) & 0x0F) << 8) ))

static int preh_init(mybmm_config_t *conf) {
	return 0;
}

static void *preh_thread(void *handle) {
	preh_info_t *info = handle;
	struct can_frame frame;
	sigset_t set;
	int idx, num, r;

	/* Ignore SIGPIPE */
	sigemptyset(&set);
	sigaddset(&set, SIGPIPE);
	sigprocmask(SIG_BLOCK, &set, NULL);

	dprintf(3,"thread started!\n");
	while(!info->stop) {
		dprintf(6,"open: %d\n", info->open);
		if (!info->open) {
			memset((void *)&info->bitmaps,0,sizeof(info->bitmaps));
			sleep(1);
			continue;
		}
		dprintf(6,"reading frame...\n");
		r = info->tp->read(info->tp_handle,0xffff,&frame,sizeof(frame));
		dprintf(6,"r: %d\n", r);
		if (r < 0) {
			memset((void *)&info->bitmaps,0,sizeof(info->bitmaps));
			sleep(1);
			continue;
		}
		if (r != sizeof(frame)) continue;
		dprintf(6,"frame.can_id: %03x\n",frame.can_id);
		/* Our range is 0x100 - 0x200 (256) */
		if (frame.can_id < 0x100 || frame.can_id > 0x1FF) continue;
//		bindump("frame",&frame,sizeof(frame));
		idx = frame.can_id - 0x100;
		dprintf(6,"idx: %03x\n", idx);
		memcpy(&info->messages[idx],&frame.data,8);
		num = idx % 32;
		info->bitmaps[idx/32] |= 1 << num;
		dprintf(6,"num: %d, bitmaps[%d]: %08x\n",num, idx/32,info->bitmaps[idx/32]);
	}
	dprintf(1,"exiting!\n");
	return 0;
}

static uint8_t *preh_get_local_can_data(preh_info_t *info, int id) {
	char what[16];
	uint32_t mask;
	int idx,num,retries;

	idx = id - 0x100;
	num = idx % 32;
	mask = 1 << num;
	dprintf(4,"id: %03x, bitmap: %04x, idx: %03x, mask: %04x\n", id, info->bitmaps[idx/32], idx, mask);
#ifdef PREH_DUMP
	retries=10;
#else
	retries=4;
#endif
	while(retries--) {
//		dprintf(1,"num: %d, bitmaps[%d]: %08x\n",num, idx/32,info->bitmaps[idx/32]);
		if ((info->bitmaps[idx/32] & mask) == 0) {
#ifndef PREH_DUMP
			dprintf(4,"bit not set, sleeping...\n");
			sleep(1);
			continue;
#else
			return 0;
#endif
		}
		if (debug >= 5) {
			sprintf(what,"%03x", id);
			bindump(what,info->messages[idx],8);
		}
		dprintf(4,"got it!\n");
		return info->messages[idx];
	}
	return 0;
}

/* Func for can data that is remote (dont use thread/messages) */
static uint8_t *preh_get_remote_can_data(preh_info_t *info, int id) {
	int retries,bytes;
	static struct can_frame frame;

	dprintf(2,"id: %03x\n", id);
	retries=5;
	while(retries--) {
		bytes = info->tp->read(info->tp_handle,id,&frame,sizeof(frame));
		dprintf(2,"bytes: %d\n", bytes);
		if (bytes < 0) return 0;
		if (bytes == sizeof(frame)) return (uint8_t *)&frame.data;
		sleep(1);
	}
	return 0;
}

static void *preh_new(mybmm_config_t *conf, ...) {
	preh_info_t *info;
	char *p;
	int i;
	va_list ap;
	mybmm_pack_t *pp;
	mybmm_module_t *tp;
	pthread_attr_t attr;

	va_start(ap,conf);
	pp = va_arg(ap,mybmm_pack_t *);
	tp = va_arg(ap,mybmm_module_t *);
	va_end(ap);

	dprintf(1,"transport: %s\n", pp->transport);
	info = calloc(1,sizeof(*info));
	if (!info) {
		perror("preh_new: malloc");
		return 0;
	}
	/* Params should be cmu #s comma delimited */
	dprintf(1,"opts: %s\n", pp->opts);
	p = pp->opts;
	for(i=0; i < 16; i++) {
		p = strele(i,",",pp->opts);
		dprintf(2,"p: %s\n", p);
		if (!strlen(p)) break;
		info->cmus[info->ncmu++] = atoi(p);
	}
	/* Save a copy of the pack */
	info->pp = pp;
	info->pp->cells = 14;
	info->tp = tp;
	dprintf(1,"pp->target: %s\n", pp->target);
	info->tp_handle = tp->new(conf,pp->target);
	if (!info->tp_handle) goto preh_new_error;

	if (strcmp(tp->name,"can") == 0) {
		/* Create a detached thread */
		if (pthread_attr_init(&attr)) {
			perror("preh_new: pthread_attr_init");
			goto preh_new_error;
		}
		if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) {
			perror("preh_new: pthread_attr_setdetachstate");
			goto preh_new_error;
		}
		info->stop = 0;
		if (pthread_create(&info->th,&attr,&preh_thread,info)) {
			perror("preh_new: pthread_create");
			goto preh_new_error;
		}
		dprintf(1,"setting func to local data\n");
		info->get_data = preh_get_local_can_data;
	} else {
		dprintf(1,"setting func to remote data\n");
		info->get_data = preh_get_remote_can_data;
	}

	return info;
preh_new_error:
	free(info);
	return 0;
}

static int preh_open(void *handle) {
	preh_info_t *info = handle;
	int r;

	dprintf(1,"opening transport\n");
	r = info->tp->open(info->tp_handle);
	if (!r) info->open = 1;
	return r;
}

static int preh_read(void *handle,...) {
	preh_info_t *info = handle;
	uint8_t *data;
	int i;
#ifndef PREH_DUMP
	int j,cell,cmu,tcount;
	float temp;

	cell = 0;
	dprintf(1,"ncmu: %d\n", info->ncmu);
	info->pp->ntemps = 0;
	for(i=0; i < info->ncmu; i++) {
		cmu = info->cmus[i];
		dprintf(2,"cmu: %d\n", cmu);
		/* Get error + balance bits */
		data = info->get_data(info,0x100 + cmu);
		if (!data) return 1;
		info->pp->error = data[0] | data[1] << 8 | data[2] << 16 | data[3] << 24;
		info->pp->balancebits = data[4] | ((data[5] * 0x0F) << 8);
		dprintf(1,"cmu: %d, error: %d, balancebits: %04x\n", i, info->pp->error, info->pp->balancebits);
		/* Get temps */
		data = info->get_data(info,0x170 + cmu);
		if (data) {
#if 0
			float min,max;
			min = 99.9;
			max = 0.0;
			for(j=0; j < 4; j++) {
				if (data[j] > 40) {
					temp = data[j] - 40;
					if (temp > max) max = temp;
					if (temp < min) min = temp;
				}
			}
			info->pp->temps[info->pp->ntemps++] = min;
			info->pp->temps[info->pp->ntemps++] = max;
#else
			/* Dont report them all, average them */
			temp = 0.0;
			tcount = 0;
			for(j=0; j < 4; j++) {
				dprintf(4,"data[%d]: %02x\n", j, data[j]);
				if (data[j] > 40) {
					temp += data[j] - 40;
					tcount++;
				}
			}
			info->pp->temps[info->pp->ntemps++] = temp/tcount;
//			tcount = _getshort(&data[4]);
//			dprintf(1,"value: %d\n", tcount);
#endif
		}
		for(j=2; j < 7; j++) {
			data = info->get_data(info,0x100 + (j << 4) + cmu);
			if (!data) continue;
			if (data[1] & 0x0F) {
				info->pp->cellvolt[cell] = _getshort(&data[0]) / 1000.0;
				dprintf(3,"cell[%02d]: %.3f\n", cell, info->pp->cellvolt[cell]);
				if (++cell >= info->pp->cells) break;
			} else {
			 	break;
			}
			if (data[3] & 0x0F) {
				info->pp->cellvolt[cell] = _getshort(&data[2]) / 1000.0;
				dprintf(3,"cell[%02d]: %.3f\n", cell, info->pp->cellvolt[cell]);
				if (++cell >= info->pp->cells) break;
			} else {
			 	break;
			}
			if (data[5] & 0x0F) {
				info->pp->cellvolt[cell] = _getshort(&data[4]) / 1000.0;
				dprintf(3,"cell[%02d]: %.3f\n", cell, info->pp->cellvolt[cell]);
				if (++cell >= info->pp->cells) break;
			} else {
			 	break;
			}
		}
	}

#else
	{
	int id,cmu,error,bal,cell;
	float voltages[15][15];

	sleep(5);
	memset(voltages,0,sizeof(voltages));
	for(i=0x100; i < 0x160; i++) {
		id = i & 0x0F0;
		cmu = i & 0x00F;
//		dprintf(1,"i: %03x, id: %03x, cmu: %03x\n", i, id, cmu);
		/* Check for errors */
		switch(id) {
		case 0x000:
			id = 0;
			break;
		case 0x020:
			id = 1;
			cell = 0;
			break;
		case 0x030:
			id = 2;
			cell = 3;
			break;
		case 0x040:
			id = 3;
			cell = 6;
			break;
		case 0x050:
			id = 4;
			cell = 9;
			break;
		case 0x060:
			id = 5;
			cell = 12;
			break;
		default:
			continue;
			break;
		}
		dprintf(2,"i: %03x, id: %03x, cell: %03x\n", i, id, cell);
//		data = &info->messages[i - 0x100][0];
		data = info->get_data(info,i);
		if (!data) continue;
		if (id == 0) {
			error = data[0] + (data[1] << 8) + (data[2] << 16) + (data[3] << 24);
			bal = ((data[5] & 0x0F ) << 8) + data[4];
			if (error || bal) dprintf(1,"cmu[%d]: error: %x, bal: %x\n", cmu, error, bal);
		} else if (id <= 5) {
			if (data[1] & 0x0F) {
				voltages[cmu][cell] = _getshort(&data[0]) / 1000.0;
				dprintf(2,"2: [%d][%d] = messages[%03x][0] = %.3f\n", cmu, cell, i, voltages[cmu][cell]);
			}
			if (data[3] & 0x0F) {
				voltages[cmu][cell+1] = _getshort(&data[2]) / 1000.0;
				dprintf(2,"2: [%d][%d] = messages[%03x][0] = %.3f\n", cmu, cell + 1, i, voltages[cmu][cell + 1]);
			}
			if (data[5] & 0x0F) {
				voltages[cmu][cell+2] = _getshort(&data[4]) / 1000.0;
				dprintf(2,"2: [%d][%d] = messages[%03x][0] = %.3f\n", cmu, cell + 2, i, voltages[cmu][cell + 2]);
			}
		}
	}

	dprintf(1,"voltages:\n");
	for(cmu=0; cmu < 15; cmu++) {
		if (!voltages[cmu][0]) continue;
		data = info->get_data(info,0x1A0 + cmu);
		if (data) {
			char str[32],*p;
			p = str;
			p += sprintf(p,"cmu[%d] id: ",cmu);
			for(i=0; i < 4; i++) {
				p += sprintf(p,"%02x ",data[i]);
			}
			printf("%s\n",str);
		}
		for(cell=0; cell < 15; cell++) {
			if (!voltages[cmu][cell]) break;
			printf("cmu[%d][%d]: %.3f\n", cmu, cell, voltages[cmu][cell]);
		}
		printf("total: %d\n",cell);
	}
	}
#endif

	return 0;

}

static int preh_close(void *handle) {
	preh_info_t *info = handle;

	if (info->open) {
		dprintf(1,"closing\n");
		info->tp->close(info->tp_handle);
		info->open = 0;
	}
	dprintf(1,"done\n");
	return 0;
}

mybmm_module_t preh_module = {
	MYBMM_MODTYPE_CELLMON,
	"preh",
	0,
	preh_init,
	preh_new,
	preh_open,
	preh_read,
	0,
	preh_close,
};
