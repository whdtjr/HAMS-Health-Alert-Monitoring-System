#include <stdio.h>
#include <pthread.h>

#include "ThreadEntry.h"
#include "mqtt.h"
#include "SharedData.h"

//스레드간 공유 자원
MQTTClient client;
pthread_mutex_t mqttClientLock = PTHREAD_MUTEX_INITIALIZER;

int main(void){
    //mqtt client 초기화
    initMQTTClient(&client);

    //테스트용
    pthread_t tid;
    pthread_create(&tid, NULL, pubEmecyThread, "test symptom"); 
    pthread_detach(tid);
    return 0;
}