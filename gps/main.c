#include <stdio.h>
#include <pthread.h>
#include <cjson/cJSON.h>
#include <fcntl.h> // open(), O_RDONLY 등을 위해 필요
#include <unistd.h> // read, close, unlink 등
#include <string.h> // strdup, memset 등
#include <sys/stat.h>

#include "MQTTClient.h"
#include "mqtt.h"
#include "ThreadEntry.h"
#include "SharedData.h"


#define PIPE_NAME "/tmp/symptom_pipe"

//스레드간 공유 자원//
MQTTClient client;
pthread_mutex_t mqttClientLock = PTHREAD_MUTEX_INITIALIZER;

Location curLocation = { 0.0 , 0.0 };
pthread_mutex_t locationLock = PTHREAD_MUTEX_INITIALIZER;

int remainedTime = 0;
pthread_mutex_t remainedTimeLock = PTHREAD_MUTEX_INITIALIZER;

ThreadStatus locationThreadStatus = {false, PTHREAD_MUTEX_INITIALIZER};
//스레드간 공유 자원//

int main(void){
    //mqtt client 초기화
    initMQTTClient(&client);

    //gpsReadThread는 프로세스가 살아있는 동안 항상 같이 실행
    pthread_t gpsReadTid;
    pthread_create(&gpsReadTid, NULL, gpsReadThread, NULL);
    pthread_detach(gpsReadTid);

    
    // Named pipe 준비
    mkfifo(PIPE_NAME, 0666);
    int fd;
    char buffer[256];

    while (1) {
        fd = open(PIPE_NAME, O_RDONLY);
        if (fd < 0) {
            perror("pipe open");
            continue;
        }

        ssize_t bytesRead = read(fd, buffer, sizeof(buffer) - 1);
        close(fd);

        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            cJSON* root = cJSON_Parse(buffer);
            if (!root) continue;

            cJSON* symptomJson = cJSON_GetObjectItem(root, "symptom");
            if (symptomJson && cJSON_IsString(symptomJson)) {
                char* symptomStr = strdup(symptomJson->valuestring);
                //pub emecy thread 시작
                pthread_t emecyThread;
                pthread_create(&emecyThread, NULL, pubEmecyThread, symptomStr);
                pthread_detach(emecyThread);

                //pub location thread 실행중인지 확인하고 시작
                pthread_mutex_lock(&locationThreadStatus.lock);
                pthread_mutex_lock(&remainedTimeLock);
                remainedTime = 360;
                pthread_mutex_unlock(&remainedTimeLock);

                if (!locationThreadStatus.running) {
                    pthread_t locationThread;
                    pthread_create(&locationThread, NULL, pubLocationThread, NULL);
                    pthread_detach(locationThread);
                }
                pthread_mutex_unlock(&locationThreadStatus.lock);
            }
            cJSON_Delete(root);
        }
    }

    unlink(PIPE_NAME);
    return 0;

}