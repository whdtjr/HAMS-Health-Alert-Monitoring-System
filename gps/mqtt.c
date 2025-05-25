#include <stdio.h>
#include "MQTTClient.h"
#include "ShareData.h"

//mqtt client를 생성하는 함수
int initMQTTClient(MQTTClient* mqttClient){
    //클라이언트 생성
    MQTTClient_create(mqttClient, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    return 1;
}

//mqtt client를 통해 브로커와 connect 하는 함수
int connectMQTTClient(MQTTClient* mqttClient){
    if(mqttClient == NULL){
        initMQTTClient(mqttClient);
    }

    if (mqttClient == NULL) {
        fprintf(stderr, "MQTTClient 포인터 자체가 NULL\n");
        return -1;
    }

    if (*mqttClient == NULL) {
        initMQTTClient(mqttClient);
    }
    //연결 옵션 설정
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;

    //브로커 연결
    int rc;
    if ((rc = MQTTClient_connect(*mqttClient, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        printf("MQTT 연결 실패. 코드: %d\n", rc);
        return -1;
    }else{
        printf("MQTT 연결 성공. 코드: %d\n", rc);
        return 1;
    }
}
//mqtt client를 disconnect 하는 함수
void disconnectAndDestroyMQTT(MQTTClient* mqttClient){
    if(mqttClient == NULL) return;
    MQTTClient_disconnect(*mqttClient, 10000);
}
//mqtt client의 리소스 정리하는 함수
void destroyMQTTClient(MQTTClient** mqttClient){
    MQTTClient_destroy(*mqttClient);
    *mqttClient = NULL;
}
