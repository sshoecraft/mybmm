
#define _GNU_SOURCE
#include <pthread.h>
#include <linux/can.h>
#include <sys/signal.h>
#include "mybmm.h"

struct power {
	float l1;
	float l2;
	float l3;
};
typedef struct power power_t;

struct si_info {
	mybmm_config_t *conf;
	mybmm_inverter_t *inv;
	pthread_t th;
	uint32_t bitmap;
	uint8_t messages[16][8];
	mybmm_module_t *tp;
	void *tp_handle;
	int stop;
	int open;
	int (*get_data)(struct si_info *info, int id, uint8_t *data, int len);
	struct {
		power_t grid;
		power_t si;
	} active;
	struct {
		power_t grid;
		power_t si;
	} reactive;
	power_t voltage;
	float frequency;
	float battery_voltage;
	float battery_current;
	float battery_temp;
	float battery_soc;
	float battery_soh;
	float battery_cvsp;
	struct {
		unsigned relay1: 1;
		unsigned relay2: 1;
		unsigned s1_relay1: 1;
		unsigned s1_relay2: 1;
		unsigned s2_relay1: 1;
		unsigned s2_relay2: 1;
		unsigned s3_relay1: 1;
		unsigned s3_relay2: 1;
		unsigned GnRn: 1;
		unsigned s1_GnRn: 1;
		unsigned s2_GnRn: 1;
		unsigned s3_GnRn: 1;
		unsigned AutoGn: 1;
		unsigned AutoLodExt: 1;
		unsigned AutoLodSoc: 1;
		unsigned Tm1: 1;
		unsigned Tm2: 1;
		unsigned ExtPwrDer: 1;
		unsigned ExtVfOk: 1;
		unsigned GdOn: 1;
		unsigned Error: 1;
		unsigned Run: 1;
		unsigned BatFan: 1;
		unsigned AcdCir: 1;
		unsigned MccBatFan: 1;
		unsigned MccAutoLod: 1;
		unsigned Chp: 1;
		unsigned ChpAdd: 1;
		unsigned SiComRemote: 1;
		unsigned OverLoad: 1;
		unsigned ExtSrcConn: 1;
		unsigned Silent: 1;
		unsigned Current: 1;
		unsigned FeedSelfC: 1;
	} bits;
	power_t load;
	uint8_t charging_proc;
	uint16_t errmsg;
	power_t ac2;
	float ac2_frequency;
	float PVPwrAt;
	float GdCsmpPwrAt;
	float GdFeedPwr;
};
typedef struct si_info si_info_t;

#define _getshort(v) (short)( ((v)[1]) << 8 | ((v)[0]) )
#define _putshort(p,v) { float tmp; *((p+1)) = ((int)(tmp = v) >> 8); *((p)) = (int)(tmp = v); }
#define _putlong(p,v) { *((p+3)) = ((int)(v) >> 24); *((p)+2) = ((int)(v) >> 16); *((p+1)) = ((int)(v) >> 8); *((p)) = (int)(v); }

static int si_init(mybmm_config_t *conf) {
        struct cfg_proctab siconf[] = {
		CFG_PROCTAB_END
	};

	cfg_get_tab(conf->cfg,siconf);
	if (debug >= 3) cfg_disp_tab(siconf,"",1);
	return 0;
}

static void *si_thread(void *handle) {
	si_info_t *info = handle;
	struct can_frame frame;
	sigset_t set;
	int r;

	/* Ignore SIGPIPE */
	sigemptyset(&set);
	sigaddset(&set, SIGPIPE);
	sigprocmask(SIG_BLOCK, &set, NULL);

	dprintf(3,"thread started!\n");
	while(!info->stop) {
		dprintf(5,"open: %d\n", info->open);
		if (!info->open) {
			memset(&info->bitmap,0,sizeof(info->bitmap));
			sleep(1);
			continue;
		}
		r = info->tp->read(info->tp_handle,0xffff,&frame,sizeof(frame));
		dprintf(7,"r: %d\n", r);
		if (r < 0) {
			memset(&info->bitmap,0,sizeof(info->bitmap));
			sleep(1);
			continue;
		}
 		if (r != sizeof(frame)) continue;
		dprintf(7,"frame.can_id: %03x\n",frame.can_id);
		if (frame.can_id < 0x300 || frame.can_id > 0x30a) continue;
//		bindump("frame",&frame,sizeof(frame));
		memcpy(&info->messages[frame.can_id - 0x300],&frame.data,8);
		info->bitmap |= 1 << (frame.can_id - 0x300);
	}
	return 0;
}

static int si_get_local_can_data(si_info_t *info, int id, uint8_t *data, int len) {
	char what[16];
	uint16_t mask;
	int idx,retries;

	dprintf(5,"id: %03x, data: %p, len: %d\n", id, data, len);
	dprintf(5,"bitmap: %04x\n", info->bitmap);
	idx = id - 0x300;
	mask = 1 << idx;
	dprintf(5,"mask: %04x\n", mask);
	retries=5;
	while(retries--) {
		if ((info->bitmap & mask) == 0) {
			dprintf(5,"bit not set, sleeping...\n");
			sleep(1);
			continue;
		}
		sprintf(what,"%03x", id);
		if (debug >= 5) bindump(what,info->messages[idx],8);
		if (len > 8) len = 8;
		memcpy(data,info->messages[idx],len);
		return 0;
	}
	return 1;
}

/* Func for can data that is remote (dont use thread/messages) */
static int si_get_remote_can_data(si_info_t *info, int id, uint8_t *data, int len) {
	int retries,bytes;
	struct can_frame frame;

	dprintf(4,"id: %03x, data: %p, len: %d\n", id, data, len);
	retries=5;
	while(retries--) {
		bytes = info->tp->read(info->tp_handle,id,&frame,sizeof(frame));
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
	struct si_info *info;
	va_list ap;
	mybmm_inverter_t *inv;
	mybmm_module_t *tp;
	pthread_attr_t attr;

	va_start(ap,conf);
	inv = va_arg(ap,mybmm_inverter_t *);
	tp = va_arg(ap,mybmm_module_t *);
	va_end(ap);

	dprintf(3,"transport: %s\n", tp->name);
	info = calloc(1,sizeof(*info));
	if (!info) {
		perror("si_new: calloc");
		return 0;
	}
	info->conf = conf;
	info->inv = inv;
	info->tp = tp;
	info->tp_handle = tp->new(conf,inv->target);
	if (!info->tp_handle) {
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
		info->stop = 0;
		if (pthread_create(&info->th,&attr,&si_thread,info)) {
			perror("si_new: pthread_create");
			goto si_new_error;
		}
		dprintf(1,"setting func to local data\n");
		info->get_data = si_get_local_can_data;
	} else {
		dprintf(1,"setting func to remote data\n");
		info->get_data = si_get_remote_can_data;
	}

	return info;
si_new_error:
	free(info);
	return 0;
}

static int si_open(void *handle) {
	si_info_t *info = handle;
	int r;

        dprintf(1,"opening transport\n");
        r = info->tp->open(info->tp_handle);
        dprintf(1,"r: %d\n",r);
        if (!r) info->open = 1;
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
	si_info_t *info = handle;
	uint8_t data[8];
	uint8_t bits;

	/* x300 Active power grid/gen */
	if (info->get_data(info,0x300,data,8)) return 1;
	info->active.grid.l1 = _getshort(&data[0]) * 100.0;
	info->active.grid.l2 = _getshort(&data[2]) * 100.0;
	info->active.grid.l3 = _getshort(&data[4]) * 100.0;
	dprintf(1,"active grid: l1: %.1f, l2: %.1f, l3: %.1f\n", info->active.grid.l1, info->active.grid.l2, info->active.grid.l3);

	/* x301 Active power Sunny Island */
	if (info->get_data(info,0x301,data,8)) return 1;
	info->active.si.l1 = _getshort(&data[0]) * 100.0;
	info->active.si.l2 = _getshort(&data[2]) * 100.0;
	info->active.si.l3 = _getshort(&data[4]) * 100.0;
	dprintf(1,"active si: l1: %.1f, l2: %.1f, l3: %.1f\n", info->active.si.l1, info->active.si.l2, info->active.si.l3);

	/* x302 Reactive power grid/gen */
	if (info->get_data(info,0x302,data,8)) return 1;
	info->reactive.grid.l1 = _getshort(&data[0]) * 100.0;
	info->reactive.grid.l2 = _getshort(&data[2]) * 100.0;
	info->reactive.grid.l3 = _getshort(&data[4]) * 100.0;
	dprintf(1,"reactive grid: l1: %.1f, l2: %.1f, l3: %.1f\n", info->reactive.grid.l1, info->reactive.grid.l2, info->reactive.grid.l3);

	/* x303 Reactive power Sunny Island */
	if (info->get_data(info,0x303,data,8)) return 1;
	info->reactive.si.l1 = _getshort(&data[0]) * 100.0;
	info->reactive.si.l2 = _getshort(&data[2]) * 100.0;
	info->reactive.si.l3 = _getshort(&data[4]) * 100.0;
	dprintf(1,"reactive si: l1: %.1f, l2: %.1f, l3: %.1f\n", info->reactive.si.l1, info->reactive.si.l2, info->reactive.si.l3);

	/* 0x304 OutputVoltage - L1 / L2 / Output Freq */
	info->get_data(info,0x304,data,8);
	info->voltage.l1 = _getshort(&data[0]) / 10.0;
	info->voltage.l2 = _getshort(&data[2]) / 10.0;
	info->voltage.l3 = _getshort(&data[4]) / 10.0;
	info->frequency = _getshort(&data[6]) / 100.0;
	dprintf(1,"voltage: l1: %.1f, l2: %.1f, l3: %.1f\n", info->voltage.l1, info->voltage.l2, info->voltage.l3);
	dprintf(1,"frequency: %.1f\n",info->frequency);

	/* 0x305 Battery voltage Battery current Battery temperature SOC battery */
	info->get_data(info,0x305,data,8);
	info->battery_voltage = _getshort(&data[0]) / 10.0;
	info->battery_current = _getshort(&data[2]) / 10.0;
	info->battery_temp = _getshort(&data[4]) / 10.0;
	info->battery_soc = _getshort(&data[6]) / 10.0;
	dprintf(1,"battery_voltage: %.1f\n", info->battery_voltage);
	dprintf(1,"battery_current: %.1f\n", info->battery_current);
	dprintf(1,"battery_temp: %.1f\n", info->battery_temp);
	dprintf(1,"battery_soc: %.1f\n", info->battery_soc);
	info->conf->battery_voltage = info->battery_voltage;

	/* 0x306 SOH battery / Charging procedure / Operating state / active Error Message / Battery Charge Voltage Set-point */
	info->get_data(info,0x306,data,8);
	info->battery_soh = _getshort(&data[0]);
	info->charging_proc = data[2];
	info->inv->state = data[3];
	info->errmsg = _getshort(&data[4]);
	info->battery_cvsp = _getshort(&data[6]) / 10.0;
	dprintf(1,"battery_soh: %.1f, charging_proc: %d, state: %d, errmsg: %d, battery_cvsp: %.1f\n",
		info->battery_soh, info->charging_proc, info->inv->state, info->errmsg, info->battery_cvsp);

	/* 0x307 Relay state / Relay function bit 1 / Relay function bit 2 / Synch-Bits */
	info->get_data(info,0x307,data,8);
#define SET(m,b) { info->bits.m = ((bits & b) != 0); dprintf(1,"bits.%s: %d\n",#m,info->bits.m); }
	bits = data[0];
	dump_bits("data[0]",bits);
	SET(relay1,   0x0001);
	SET(relay2,   0x0002);
	SET(s1_relay1,0x0004);
	SET(s1_relay2,0x0008);
	SET(s2_relay1,0x0010);
	SET(s2_relay1,0x0020);
	SET(s3_relay1,0x0040);
	SET(s3_relay1,0x0080);
	bits = data[1];
	dump_bits("data[1]",bits);
	SET(GnRn,     0x0001);
	SET(s1_GnRn,  0x0002);
	SET(s2_GnRn,  0x0004);
	SET(s3_GnRn,  0x0008);
	bits = data[2];
	dump_bits("data[2]",bits);
	SET(AutoGn,    0x0001);
	SET(AutoLodExt,0x0002);
	SET(AutoLodSoc,0x0004);
	SET(Tm1,       0x0008);
	SET(Tm2,       0x0010);
	SET(ExtPwrDer, 0x0020);
	SET(ExtVfOk,   0x0040);
	SET(GdOn,      0x0080);
	bits = data[3];
	dump_bits("data[3]",bits);
	SET(Error,     0x0001);
	SET(Run,       0x0002);
	SET(BatFan,    0x0004);
	SET(AcdCir,    0x0008);
	SET(MccBatFan, 0x0010);
	SET(MccAutoLod,0x0020);
	SET(Chp,       0x0040);
	SET(ChpAdd,    0x0080);
	bits = data[4];
	dump_bits("data[4]",bits);
	SET(SiComRemote,0x0001);
	SET(OverLoad,  0x0002);
	bits = data[5];
	dump_bits("data[5]",bits);
	SET(ExtSrcConn,0x0001);
	SET(Silent,    0x0002);
	SET(Current,   0x0004);
	SET(FeedSelfC, 0x0008);

	/* 0x308 TotLodPwr */
	info->get_data(info,0x308,data,8);
	info->load.l1 = _getshort(&data[0]) * 100.0;
	info->load.l2 = _getshort(&data[2]) * 100.0;
	info->load.l3 = _getshort(&data[4]) * 100.0;
	dprintf(1,"load: l1: %.1f, l2: %.1f, l3: %.1f\n",info->load.l1, info->load.l2, info->load.l3);

	/* 0x309 AC2 Voltage L1 / AC2 Voltage L2 / AC2 Voltage L3 / AC2 Frequency */
	info->get_data(info,0x309,data,8);
	info->ac2.l1 = _getshort(&data[0]) / 10.0;
	info->ac2.l2 = _getshort(&data[2]) / 10.0;
	info->ac2.l3 = _getshort(&data[4]) / 10.0;
	info->ac2_frequency = _getshort(&data[6]) / 100.0;
	dprintf(1,"ac2: l1: %.1f, l2: %.1f, l3: %.1f\n",info->ac2.l1, info->ac2.l2, info->ac2.l3);
	dprintf(1,"ac2 frequency: %.1f\n",info->ac2_frequency);

	/* 0x30A PVPwrAt / GdCsmpPwrAt / GdFeedPwr */
	info->get_data(info,0x30a,data,8);
	info->PVPwrAt = _getshort(&data[0]) / 10.0;
	info->GdCsmpPwrAt = _getshort(&data[0]) / 10.0;
	info->GdFeedPwr = _getshort(&data[0]) / 10.0;
	dprintf(1,"PVPwrAt: %.1f\n", info->PVPwrAt);
	dprintf(1,"GdCsmpPwrAt: %.1f\n", info->GdCsmpPwrAt);
	dprintf(1,"GdFeedPwr: %.1f\n", info->GdFeedPwr);
	return 0;
}

#if 0
static int si_get_info(si_info_t *info) {
	/* not impl */
	return -1;
}

static int si_read(void *handle,...) {
	si_info_t *info = handle;
	int r;

	/* CAN or 485? */
	dprintf(3,"tp->name: %s\n", info->tp->name);
	if (strcmp(info->tp->name,"can")==0) 
		r = si_get_can_info(info);
	else
		r = si_get_info(info);
	dprintf(3,"r: %d\n", r);
	return r;
}
#endif

#define SI_GEN_ENABLE 0x10
#define SI_GRID_ENABLE 0x20
#define SI_CAN_SWITCH 0x80

void si_grid_control(void *handle, int value) {
	si_info_t *info = handle;
	unsigned char b;

	b = 0;
	if (mybmm_check_state(info->conf,MYBMM_GEN_ENABLE)) {
		b |= SI_GEN_ENABLE;
		b |= SI_CAN_SWITCH;
	}
	if (mybmm_check_state(info->conf,MYBMM_GRID_ENABLE)) {
		b |= SI_GRID_ENABLE;
		b |= SI_CAN_SWITCH;
	}
	info->tp->write(info->tp_handle,0x354,&b,1);
}

static int si_write(void *handle,...) {
	si_info_t *info = handle;
	mybmm_config_t *conf = info->conf;
	uint8_t data[8];

//	conf->charge_voltage = info->battery_voltage;
	dprintf(1,"0x351: charge_voltage: %.1f, charge_amps: %.1f, discharge_amps: %.1f, discharge_voltage: %.1f\n",
		conf->charge_voltage, conf->charge_amps, conf->discharge_amps, conf->discharge_voltage);
	_putshort(&data[0],(conf->charge_voltage * 10.0));
	_putshort(&data[2],(conf->charge_amps * 10.0));
	_putshort(&data[4],(conf->discharge_amps * 10.0));
	_putshort(&data[6],(conf->discharge_voltage * 10.0));
	if (info->tp->write(info->tp_handle,0x351,&data,8) < 0) {
		dprintf(1,"write failed!\n");
		return 1;
	}

//	conf->soc = 100.0;
	dprintf(1,"0x355: SOC: %.1f, SOH: %.1f\n", conf->soc, conf->soh);
	_putshort(&data[0],conf->soc);
	_putshort(&data[2],conf->soh);
	_putlong(&data[4],(conf->soc * 100.0));
	if (info->tp->write(info->tp_handle,0x355,&data,8) < 0) {
		dprintf(1,"write failed!\n");
		return 1;
	}

	/* Alarms/Warnings */
	memset(data,0,sizeof(data));
	if (info->tp->write(info->tp_handle,0x35A,&data,8) < 0) return 1;

	/* Events */
	memset(data,0,sizeof(data));
	if (info->tp->write(info->tp_handle,0x35B,&data,8)) return 1;

	/* MFG Name */
	memset(data,' ',sizeof(data));
#define MFG_NAME "RSW"
	memcpy(data,MFG_NAME,strlen(MFG_NAME));
	if (info->tp->write(info->tp_handle,0x35E,&data,8) < 0) return 1;

	/* 0x35F - Bat-Type / BMS Version / Bat-Capacity / reserved Manufacturer ID */
	_putshort(&data[0],1);
	/* major.minor.build.revision - encoded as MmBBr 10142 = 1.0.14.2 */
	_putshort(&data[2],10000);
	_putshort(&data[4],conf->capacity);
	_putshort(&data[6],1);
	if (info->tp->write(info->tp_handle,0x35F,&data,8) < 0) return 1;

	return 0;
}

int si_shutdown(void *handle) {
	si_info_t *info = handle;
	info->tp->write(info->tp_handle,0x00F,0,0);
	return 0;
}

static int si_close(void *handle) {
	si_info_t *info = handle;

	dprintf(1,"open: %d\n",info->open);
	if (info->open) {
		dprintf(1,"closing\n");
		info->tp->close(info->tp_handle);
		info->open = 0;
	}
	return 0;
}

static int si_control(void *handle,...) {
//	si_info_t *info = handle;
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
