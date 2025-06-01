#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <cjson/cJSON.h>
#include "MQTTClient.h"
#include "ThreadEntry.h"
#include "SharedData.h"

void setThreadStatus(bool status){
    pthread_mutex_lock(&locationThreadStatus.lock);
    locationThreadStatus.running = status;
    pthread_mutex_unlock(&locationThreadStatus.lock);
}

void* pubLocationThread(void* arg) {
   printf("pubLocationThread 시작\n");
   setThreadStatus(true);

   pthread_mutex_lock(&remainedTimeLock);

   while(remainedTime > 0){
        pthread_mutex_unlock(&remainedTimeLock);

        pthread_mutex_lock(&mqttClientLock);
        // 브로커 서버에 연결이 되어 있는지 확인
        if (MQTTClient_isConnected(client) == 0) {
             printf("브로커 서버에 연결 시도\n");
                if (connectMQTTClient(&client) == -1) {
                    printf("브로커 서버 연결 실패\n");
                    pthread_mutex_unlock(&mqttClientLock);
                    setThreadStatus(false);
                    pthread_exit(NULL);
                } else {
                    printf("브로커 서버 연결 성공\n");
                }
        }else{
                printf("브로커 서버에 이미 연결된 상태\n");
        }

            cJSON* root = cJSON_CreateObject();
            cJSON_AddStringToObject(root, "devId", DEVICEID);
            cJSON_AddStringToObject(root, "type", "location");
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
            MQTTClient_publishMessage(client, LOCATIONTOPIC, &pubmsg, &token);
            MQTTClient_waitForCompletion(client, token, TIMEOUT);
            printf("운전자 현재 위치 완료: %s\n", json_str);

            pthread_mutex_unlock(&mqttClientLock); 

            free(json_str);  
            pthread_mutex_lock(&remainedTimeLock);
            remainedTime--;
            pthread_mutex_unlock(&remainedTimeLock);
            sleep(5);// 5초마다 반복
   }

    setThreadStatus(false);
    pthread_exit(NULL); 
}