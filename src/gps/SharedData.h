// SharedData.h
#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#include <stdbool.h>
#include <pthread.h>
#include "MQTTClient.h"

#define ADDRESS     "tcp://192.168.219.103:1883"       // 브로커 주소
#define CLIENTID    "dev1-publisher"
#define DEVICEID "dev1"
#define EMECYTOPIC  "msg/emecy/" DEVICEID
#define LOCATIONTOPIC  "msg/location/" DEVICEID
#define QOS         1
#define TIMEOUT     10000L

typedef struct {
    bool running;
    pthread_mutex_t lock;
} ThreadStatus;

typedef struct {
    double lat;
    double lng;
} Location;

typedef struct {
    char* devId;
    char* symptom;
    Location location;
} EmecyReport;

typedef struct{
    char* devId;
    Location location;
} DriverLocation;

//스레드간 공유 자원//
extern Location curLocation;
extern pthread_mutex_t locationLock;

extern MQTTClient client;
extern pthread_mutex_t mqttClientLock;

extern int remainedTime;
extern pthread_mutex_t remainedTimeLock;

extern ThreadStatus locationThreadStatus;
//스레드간 공유 자원//

#endif // SHARED_DATA_H