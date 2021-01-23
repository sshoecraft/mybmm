
#include <string.h>
#include "MQTTClient.h"
#include "mqtt.h"
#include "debug.h"

#define TIMEOUT 1000L

int mqtt_send(char *address, char *clientid, char *message, char *topic) {
	MQTTClient client;
	MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
	MQTTClient_message pubmsg = MQTTClient_message_initializer;
	MQTTClient_deliveryToken token;
	int rc;

	MQTTClient_create(&client, address, clientid, MQTTCLIENT_PERSISTENCE_NONE, NULL);
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
	if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
		printf("Failed to connect, return code %d\n", rc);
		return 1;
	}
	dprintf(1,"client: %s, message: %s, topic: %s\n", clientid, message, topic);
	pubmsg.payload = message;
	pubmsg.payloadlen = strlen(message);
	pubmsg.qos = 1;
	pubmsg.retained = 1;
	MQTTClient_publishMessage(client, topic, &pubmsg, &token);
	rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
	dprintf(1,"delivered message.\n");
	MQTTClient_disconnect(client, TIMEOUT);
	MQTTClient_destroy(&client);
	return rc;
}

