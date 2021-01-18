
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>
#include <dlfcn.h>
#include "mybmm.h"
#include "log.h"

int debug = 0;

void display(struct mybmm_config *conf);

void usage(char *name) {
	printf("usage: %s [-acjJrwlh] [-f filename] [-b <bluetooth mac addr | -i <ip addr>] [-o output file]\n",name);
	printf("arguments:\n");
#ifdef DEBUG
	printf("  -d <#>		debug output\n");
#endif
	printf("  -h		this output\n");
	printf("  -t <transport:target> transport & target\n");
	printf("  -e <opts>	transport-specific opts\n");
}

int main(int argc, char **argv) {
	int opt;
	char type[MYBMM_MODULE_NAME_LEN+1],transport[MYBMM_MODULE_NAME_LEN+1],target[MYBMM_TARGET_LEN+1],*opts;
	mybmm_config_t *conf;
	mybmm_pack_t *pp;
	worker_pool_t *pool;

	opts = 0;
	type[0] = transport[0] = target[0] = 0;
	while ((opt=getopt(argc, argv, "d:p:e:h")) != -1) {
		switch (opt) {
		case 'd':
			debug=atoi(optarg);
			break;
                case 'p':
			strncpy(type,strele(0,":",optarg),sizeof(type)-1);
			strncpy(transport,strele(1,":",optarg),sizeof(transport)-1);
			strncpy(target,strele(2,":",optarg),sizeof(target)-1);
			if (!strlen(type) || !strlen(transport) || !strlen(target)) {
				printf("error: format is type:transport:target\n");
				usage(argv[0]);
				return 1;
			}
			break;
		case 'e':
			opts = optarg;
			break;
		case 'h':
		default:
			usage(argv[0]);
			exit(0);
                }
	}

	/* Create the config */
	conf = calloc(sizeof(*conf),1);
	if (!conf) {
		perror("calloc conf");
		return 1;
	}
	conf->modules = list_create();
	conf->packs = list_create();

	printf("opening log...\n");
	log_open("mybmm","mybmm.log",LOG_TIME|LOG_INFO|LOG_WARNING|LOG_ERROR|LOG_SYSERR|LOG_DEBUG);
	log_write(LOG_DEBUG,"testing");
	printf("exiting...\n");
	return 0;
}
