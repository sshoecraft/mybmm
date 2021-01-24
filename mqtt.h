
#ifndef __MYBMM_MQTT_H
#define __MYBMM_MQTT_H

#include <MQTTClient.h>

struct mqtt_session;
typedef struct mqtt_session mqtt_session_t;

int mqtt_init(mybmm_config_t *conf);
struct mqtt_session *mqtt_new(char *address, char *clientid, char *topic);
int mqtt_connect(mqtt_session_t *s, int interval);
int mqtt_disconnect(mqtt_session_t *s, int timeout);
int mqtt_destroy(mqtt_session_t *s);
int mqtt_send(mqtt_session_t *s, char *message, int timeout);
int mqtt_sub(mqtt_session_t *s, char *topic);
int mqtt_setcb(mqtt_session_t *s, void *ctx, MQTTClient_connectionLost *cl, MQTTClient_messageArrived *ma, MQTTClient_deliveryComplete *dc);

#if 0
typedef void (mqtt_cl_t)(void *ctx, char *cause);
typedef int mqtt_ma_t(void *ctx, char *topic, int tlen, MQTTClient_message *message);
typedef void mqtt_dc_t(void *ctx, MQTTClient_deliveryToken dt);
int mqtt_setcb(mqtt_session_t *s, void *ctx, mqtt_cl_t *cl,  mqtt_ma_t *ma, mqtt_dc_t *dc);
#endif

int mqtt_fullsend(char *address, char *clientid, char *message, char *topic);

#endif
