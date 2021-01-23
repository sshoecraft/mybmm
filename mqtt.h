
#ifndef __MYBMM_MQTT_H
#define __MYBMM_MQTT_H

struct mqtt_session;
typedef struct mqtt_session mqtt_session_t;

int mqtt_init(mybmm_config_t *conf);
struct mqtt_session *mqtt_new(char *address, char *clientid, char *topic);
int mqtt_connect(mqtt_session_t *s, int interval);
int mqtt_disconnect(mqtt_session_t *s, int timeout);
int mqtt_destroy(mqtt_session_t *s);
int mqtt_send(mqtt_session_t *s, char *message, int timeout);

int mqtt_fullsend(char *address, char *clientid, char *message, char *topic);

#endif
