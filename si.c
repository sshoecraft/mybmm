
#define _GNU_SOURCE
#include <pthread.h>
#include <linux/can.h>
#include <sys/signal.h>
#include "mybmm.h"
#include "si.h"

#define _getshort(v) (short)( ((v)[1]) << 8 | ((v)[0]) )
#define _putshort(p,v) { float tmp; *((p+1)) = ((int)(tmp = v) >> 8); *((p)) = (int)(tmp = v); }
#define _putlong(p,v) { *((p+3)) = ((int)(v) >> 24); *((p)+2) = ((int)(v) >> 16); *((p+1)) = ((int)(v) >> 8); *((p)) = (int)(v); }

static int si_init(mybmm_config_t *conf) {
#if 0
        struct cfg_proctab siconf[] = {
		CFG_PROCTAB_END
	};

	cfg_get_tab(conf->cfg,siconf);
	if (debug >= 3) cfg_disp_tab(siconf,"",1);
#endif
	return 0;
}

static void *si_thread(void *handle) {
	si_session_t *s = handle;
	struct can_frame frame;
	sigset_t set;
	int r;

	/* Ignore SIGPIPE */
	sigemptyset(&set);
	sigaddset(&set, SIGPIPE);
	sigprocmask(SIG_BLOCK, &set, NULL);

	dprintf(3,"thread started!\n");
	while(!s->stop) {
		dprintf(7,"open: %d\n", s->open);
		if (!s->open) {
			memset(&s->bitmap,0,sizeof(s->bitmap));
			sleep(1);
			continue;
		}
		r = s->tp->read(s->tp_handle,0xffff,&frame,sizeof(frame));
		dprintf(7,"r: %d\n", r);
		if (r < 0) {
			memset(&s->bitmap,0,sizeof(s->bitmap));
			sleep(1);
			continue;
		}
 		if (r != sizeof(frame)) continue;
		dprintf(7,"frame.can_id: %03x\n",frame.can_id);
		if (frame.can_id < 0x300 || frame.can_id > 0x30a) continue;
//		bindump("frame",&frame,sizeof(frame));
		memcpy(&s->messages[frame.can_id - 0x300],&frame.data,8);
		s->bitmap |= 1 << (frame.can_id - 0x300);
	}
	return 0;
}

static int si_get_local_can_data(si_session_t *s, int id, uint8_t *data, int len) {
	char what[16];
	uint16_t mask;
	int idx,retries;

	dprintf(5,"id: %03x, data: %p, len: %d\n", id, data, len);
	dprintf(5,"bitmap: %04x\n", s->bitmap);
	idx = id - 0x300;
	mask = 1 << idx;
	dprintf(5,"mask: %04x\n", mask);
	retries=5;
	while(retries--) {
		if ((s->bitmap & mask) == 0) {
			dprintf(5,"bit not set, sleeping...\n");
			sleep(1);
			continue;
		}
		sprintf(what,"%03x", id);
		if (debug >= 5) bindump(what,s->messages[idx],8);
		if (len > 8) len = 8;
		memcpy(data,s->messages[idx],len);
		return 0;
	}
	return 1;
}

/* Func for can data that is remote (dont use thread/messages) */
static int si_get_remote_can_data(si_session_t *s, int id, uint8_t *data, int len) {
	int retries,bytes;
	struct can_frame frame;

	dprintf(4,"id: %03x, data: %p, len: %d\n", id, data, len);
	retries=5;
	while(retries--) {
		bytes = s->tp->read(s->tp_handle,id,&frame,sizeof(frame));
		dprintf(2,"bytes: %d\n", bytes);
		if (bytes < 0) return 1;
		if (bytes == sizeof(frame)) {
			memset(data,0,len);
			memcpy(data,&frame.data,frame.can_dlc);
//			bindump("data",data,frame.can_dlc);
			break;
		}
		sleep(1);
	}
	dprintf(4,"returning: %d\n", (retries > 0 ? 0 : 1));
	return (retries > 0 ? 0 : 1);
}

static void *si_new(mybmm_config_t *conf, ...) {
	si_session_t *s;
	va_list ap;
	mybmm_inverter_t *inv;
	mybmm_module_t *tp;
	pthread_attr_t attr;

	va_start(ap,conf);
	inv = va_arg(ap,mybmm_inverter_t *);
	tp = va_arg(ap,mybmm_module_t *);
	va_end(ap);

	dprintf(3,"transport: %s\n", tp->name);
	s = calloc(1,sizeof(*s));
	if (!s) {
		perror("si_new: calloc");
		return 0;
	}
	s->conf = conf;
	s->inv = inv;
	s->tp = tp;
	s->tp_handle = tp->new(conf,inv->target);
	if (!s->tp_handle) {
		goto si_new_error;
	}

	/* Only do the thread if using can directly */
	if (strcmp(tp->name,"can") == 0) {
		/* Create a detached thread */
		if (pthread_attr_init(&attr)) {
			perror("si_new: pthread_attr_init");
			goto si_new_error;
		}
		if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) {
			perror("si_new: pthread_attr_setdetachstate");
			goto si_new_error;
		}
		s->stop = 0;
		if (pthread_create(&s->th,&attr,&si_thread,s)) {
			perror("si_new: pthread_create");
			goto si_new_error;
		}
		dprintf(1,"setting func to local data\n");
		s->get_data = si_get_local_can_data;
	} else {
		dprintf(1,"setting func to remote data\n");
		s->get_data = si_get_remote_can_data;
	}

	return s;
si_new_error:
	free(s);
	return 0;
}

static int si_open(void *handle) {
	si_session_t *s = handle;
	int r;

        dprintf(1,"opening transport\n");
        r = s->tp->open(s->tp_handle);
        dprintf(1,"r: %d\n",r);
        if (!r) s->open = 1;
        return r;
}

void dump_bits(char *label, uint8_t bits) {
	register uint8_t i,mask;
	char bitstr[9];

	i = 0;
	mask = 1;
	while(mask) {
		bitstr[i++] = (bits & mask ? '1' : '0');
		mask <<= 1;
	}
	bitstr[i] = 0;
	dprintf(1,"%s(%02x): %s\n",label,bits,bitstr);
}

static int si_read(void *handle,...) {
	si_session_t *s = handle;
	mybmm_inverter_t *inv = s->inv;
	uint8_t data[8];

	/* x300 Active power grid/gen L1/L2/L3 */
	if (s->get_data(s,0x300,data,8)) return 1;
	inv->grid_power = (_getshort(&data[0]) * 100.0) + (_getshort(&data[2]) * 100.0) + (_getshort(&data[4]) * 100.0);
	dprintf(1,"grid_power: %3.2f\n",inv->grid_power);

	/* 0x305 Battery voltage Battery current Battery temperature SOC battery */
	if (s->get_data(s,0x305,data,8)) return 1;
	inv->battery_voltage = _getshort(&data[0]) / 10.0;
	inv->battery_amps = _getshort(&data[2]) / 10.0;
	dprintf(1,"battery_voltage: %2.2f, battery_amps: %2.2f\n", inv->battery_voltage, inv->battery_amps);
	inv->battery_power = inv->battery_amps * inv->battery_voltage;
	dprintf(1,"battery_power: %3.2f\n",inv->battery_power);

	/* 0x308 TotLodPwr L1/L2/L3 */
	s->get_data(s,0x308,data,8);
	inv->load_power = (_getshort(&data[0]) * 100.0) + (_getshort(&data[2]) * 100.0) + (_getshort(&data[4]) * 100.0);
	dprintf(1,"load_power: %3.2f\n",inv->load_power);

	/* 0x30A PVPwrAt / GdCsmpPwrAt / GdFeedPwr */
	s->get_data(s,0x30a,data,8);
	inv->site_power = _getshort(&data[0]) * 100.0;
	dprintf(1,"site_power: %3.2f\n",inv->site_power);
	return 0;
}

#define SI_GEN_ENABLE 0x10
#define SI_GRID_ENABLE 0x20
#define SI_CAN_SWITCH 0x80

void si_grid_control(void *handle, int value) {
	si_session_t *s = handle;
	unsigned char b;

	b = 0;
	if (mybmm_check_state(s->conf,MYBMM_GEN_ENABLE)) {
		b |= SI_GEN_ENABLE;
		b |= SI_CAN_SWITCH;
	}
	if (mybmm_check_state(s->conf,MYBMM_GRID_ENABLE)) {
		b |= SI_GRID_ENABLE;
		b |= SI_CAN_SWITCH;
	}
	s->tp->write(s->tp_handle,0x354,&b,1);
}

static int si_write(void *handle,...) {
	si_session_t *s = handle;
	mybmm_config_t *conf = s->conf;
	uint8_t data[8];

	dprintf(1,"0x351: charge_voltage: %3.2f, charge_amps: %3.2f, discharge_voltage: %3.2f, discharge_amps: %3.2f\n",
		conf->charge_voltage, conf->charge_amps, conf->discharge_voltage, conf->discharge_amps);
	_putshort(&data[0],(conf->charge_voltage * 10.0));
	_putshort(&data[2],(conf->charge_amps * 10.0));
	_putshort(&data[4],(conf->discharge_amps * 10.0));
	_putshort(&data[6],(conf->discharge_voltage * 10.0));
	if (s->tp->write(s->tp_handle,0x351,&data,8) < 0) {
		dprintf(1,"write failed!\n");
		return 1;
	}

	dprintf(1,"0x355: SOC: %.1f, SOH: %.1f\n", conf->soc, conf->soh);
	_putshort(&data[0],conf->soc);
	_putshort(&data[2],conf->soh);
	_putlong(&data[4],(conf->soc * 100.0));
	if (s->tp->write(s->tp_handle,0x355,&data,8) < 0) {
		dprintf(1,"write failed!\n");
		return 1;
	}

	dprintf(1,"0x356: battery_voltage: %.1f, battery_amps: %.1f, battery_temp: %.1f\n",
		conf->battery_voltage, conf->battery_amps, conf->battery_temp);
	_putshort(&data[0],conf->battery_voltage * 10.0);
	_putshort(&data[2],conf->battery_amps * 10.0);
	_putlong(&data[4],conf->battery_temp * 10.0);
	if (s->tp->write(s->tp_handle,0x356,&data,8) < 0) {
		dprintf(1,"write failed!\n");
		return 1;
	}

	/* Alarms/Warnings */
	memset(data,0,sizeof(data));
	if (s->tp->write(s->tp_handle,0x35A,&data,8) < 0) return 1;

	/* Events */
	memset(data,0,sizeof(data));
	if (s->tp->write(s->tp_handle,0x35B,&data,8)) return 1;

	/* MFG Name */
	memset(data,' ',sizeof(data));
#define MFG_NAME "RSW"
	memcpy(data,MFG_NAME,strlen(MFG_NAME));
	if (s->tp->write(s->tp_handle,0x35E,&data,8) < 0) return 1;

	/* 0x35F - Bat-Type / BMS Version / Bat-Capacity / reserved Manufacturer ID */
	_putshort(&data[0],1);
	/* major.minor.build.revision - encoded as MmBBr 10142 = 1.0.14.2 */
	_putshort(&data[2],10000);
	_putshort(&data[4],conf->capacity);
	_putshort(&data[6],1);
	if (s->tp->write(s->tp_handle,0x35F,&data,8) < 0) return 1;

	return 0;
}

int si_shutdown(void *handle) {
	si_session_t *s = handle;
	s->tp->write(s->tp_handle,0x00F,0,0);
	return 0;
}

static int si_close(void *handle) {
	si_session_t *s = handle;

	dprintf(1,"open: %d\n",s->open);
	if (s->open) {
		dprintf(1,"closing\n");
		s->tp->close(s->tp_handle);
		s->open = 0;
	}
	return 0;
}

static int si_control(void *handle,...) {
//	si_session_t *s = handle;
	va_list ap;
	int op,action;

	va_start(ap,handle);
	op = va_arg(ap,int);
	dprintf(1,"op: %d\n", op);
	switch(op) {
	case MYBMM_INVERTER_GRID_CONTROL:
		action = va_arg(ap,int);
		dprintf(1,"action: %d\n", action);
		break;
	}
	va_end(ap);

	return 0;
}

mybmm_module_t si_module = {
	MYBMM_MODTYPE_INVERTER,
	"si",
	MYBMM_INVERTER_GRID_CONTROL  | MYBMM_INVERTER_GEN_CONTROL | MYBMM_INVERTER_POWER_CONTROL,
	si_init,
	si_new,
	si_open,
	si_read,
	si_write,
	si_close,
	0,
	si_shutdown,
	si_control,
};
