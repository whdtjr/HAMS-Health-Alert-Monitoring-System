#ifndef MQTT_H
#define MQTT_H
#include "MQTTClient.h"

int initMQTTClient(MQTTClient* mqttClient);
int connectMQTTClient(MQTTClient* mqttClient);
void disconnectAndDestroyMQTT(MQTTClient* mqttClient);
void destroyMQTTClient(MQTTClient** mqttClient);
#endif // MQTT_H
