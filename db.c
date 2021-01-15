

#include "mybmm.h"

#include <sqlite3.h>
#include <stdio.h>

int tst3(void) {
    
    sqlite3 *db;
    sqlite3_stmt *res;
    
    int rc = sqlite3_open(":memory:", &db);
    
    if (rc != SQLITE_OK) {
        
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        
        return 1;
    }
    
    rc = sqlite3_prepare_v2(db, "SELECT SQLITE_VERSION()", -1, &res, 0);    
    
    if (rc != SQLITE_OK) {
        
        fprintf(stderr, "Failed to fetch data: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        
        return 1;
    }    
    
    rc = sqlite3_step(res);
    
    if (rc == SQLITE_ROW) {
        printf("%s\n", sqlite3_column_text(res, 0));
    }
    
    sqlite3_finalize(res);
    sqlite3_close(db);
    
    return 0;
}

int db_init(mybmm_config_t *conf, char *name) {
	char db_type[32];
	struct cfg_proctab dbconf[] = {
		{ name, "type", "DB Type", DATA_TYPE_STRING, &db_type, sizeof(db_type), "" },
		CFG_PROCTAB_END
	};

	cfg_get_tab(conf->cfg,dbconf);
#ifdef DEBUG
	if (debug) cfg_disp_tab(dbconf,0,1);
#endif
	mybmm_load_module(conf,db_type,MYBMM_MODTYPE_DB);
	return 0;
}
