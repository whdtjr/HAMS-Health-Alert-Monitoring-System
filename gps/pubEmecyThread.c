#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <cjson/cJSON.h>

#include "ThreadEntry.h"
#include "SharedData.h"

void* pubEmecyThread(void* arg) {
   printf("pubEmecyThread 시작 arg: %s\n",(char*) arg);

   char symptom[256];  
   strcpy(symptom, (char *) arg); 
   printf("pubEmecyThread symptom: %s\n", symptom);

   pthread_mutex_lock(&mqttClientLock);

    // 브로커 서버에 연결이 되어 있는지 확인
    if (MQTTClient_isConnected(client) == 0) {
        printf("브로커 서버에 연결 시도\n");
        if (connectMQTTClient(&client) == -1) {
            printf("브로커 서버 연결 실패\n");
            pthread_mutex_unlock(&mqttClientLock);
            pthread_exit(NULL);
        } else {
            printf("브로커 서버 연결 성공\n");
        }
    }else{
        printf("브로커 서버에 이미 연결된 상태\n");
    }
    
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "devId", DEVICEID);
    cJSON_AddStringToObject(root, "type", "emecy");
    cJSON_AddStringToObject(root, "symptom", symptom);
    pthread_mutex_lock(&locationLock);
    cJSON_AddNumberToObject(root, "lat", curLocation.lat);
    cJSON_AddNumberToObject(root, "lng", curLocation.lng);
    pthread_mutex_unlock(&locationLock);
    char* json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    pubmsg.payload = json_str;
    pubmsg.payloadlen = (int)strlen(json_str);
    pubmsg.qos = QOS;
    pubmsg.retained = 0;

    // 5. 메시지 전송
    MQTTClient_deliveryToken token;
    MQTTClient_publishMessage(client, EMECYTOPIC, &pubmsg, &token);
    MQTTClient_waitForCompletion(client, token, TIMEOUT);
    printf("응급 메시지 전송 완료: %s\n", json_str);

    pthread_mutex_unlock(&mqttClientLock); 

    free(json_str);  

    pthread_exit(NULL); 
}