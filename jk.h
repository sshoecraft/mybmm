
#ifndef __JK_H
#define __JK_H

struct jk_session {
	mybmm_pack_t *pp;		/* Our pack info */
	mybmm_module_t *tp;		/* Our transport */
	void *tp_handle;		/* Our transport handle */
};
typedef struct jk_session jk_session_t;

struct jk_info {
	char model[32];
	char hwvers[16];
	char swvers[16];
	char uptime[24];
	char device[32];
	char pass[8];
	float voltage;
	float current;
	unsigned short protectbits;
	unsigned short state;
	unsigned char strings;			/* the number of battery strings */
	float cellvolt[32];			/* Cell voltages */
	float cellres[32];			/* Cell resistances */
	float cell_total;			/* sum of all cells */
	float cell_min;				/* lowest cell value */
	float cell_max;				/* highest cell value */
	float cell_diff;			/* diff from lowest to highest */
	float cell_avg;				/* avergae cell value */
	unsigned char probes;			/* the number of NTC (temp) probes */
	float temps[6];				/* Temps */
};
typedef struct jk_info jk_info_t;

#endif
