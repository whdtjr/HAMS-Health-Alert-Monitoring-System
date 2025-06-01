// sendFeature.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <cjson/cJSON.h>

#include "SharedData.h"
#include "sendFeature.h"

#define BUF_SIZE 512

void print_sent_data(PPGData* data) {
    char* timestamp_str = ctime(&data->timestamp);
    if (timestamp_str[strlen(timestamp_str) - 1] == '\n') timestamp_str[strlen(timestamp_str) - 1] = '\0';

    // printf("[전송됨] Time: %s | Signal: %d | BPM: %d | IBI: %d | SDNN: %.2lf | RMSSD: %.2lf | PNN50: %.2lf\n",
    //        timestamp_str, data->signal, data->bpm, data->ibi, data->sdnn, data->rmssd, data->pnn50);
}


void* send_feature_thread(void* arg) {

    char* server_ip = ((char**)arg)[0];
    int server_port = atoi(((char**)arg)[1]);

    printf("[Info] PPG data Sending Thread Start\n");
    
 
    int sock;
    struct sockaddr_in serv_addr;

    // 1. 소켓 생성 및 연결
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        printf("socket() error");
        perror("socket() error");
        pthread_exit(NULL);
    }


    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(server_ip);
    serv_addr.sin_port = htons(server_port);


    // //서버 주소 구조체 설정
    // memset(&serv_addr, 0, sizeof(serv_addr));
    // serv_addr.sin_family = AF_INET;
    // serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    // serv_addr.sin_port = htons(atoi(argv[2]));



    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        // printf("connect()error");
        // perror("connect() error");
        close(sock);
        pthread_exit(NULL);
    }
    printf("Connect Success\n");
    cJSON* idJson = cJSON_CreateObject();
    cJSON_AddStringToObject(idJson, "id", "hrv");
    char* idJsonString = cJSON_PrintUnformatted(idJson);
    cJSON_Delete(idJson);
    send(sock, idJsonString, strlen(idJsonString), 0);
    free(idJsonString);

    // 2. 10초마다 latest_data를 JSON으로 만들어 전송
    while (1) {
        fflush(stdout);  
        pthread_mutex_lock(&data_lock);
        PPGData copy = latest_data;
        pthread_mutex_unlock(&data_lock);

        cJSON* root = cJSON_CreateObject();
        cJSON_AddNumberToObject(root, "bpm", copy.bpm);
        cJSON_AddNumberToObject(root, "ibi", copy.ibi);
        cJSON_AddNumberToObject(root, "signal", copy.signal);
        cJSON_AddNumberToObject(root, "sdnn", copy.sdnn);
        cJSON_AddNumberToObject(root, "rmssd", copy.rmssd);
        cJSON_AddNumberToObject(root, "pnn50", copy.pnn50);
        cJSON_AddNumberToObject(root, "timestamp", (double)copy.timestamp);

        char* json_str = cJSON_PrintUnformatted(root);
        cJSON_Delete(root);

        send(sock, json_str, strlen(json_str), 0);
        free(json_str);

        print_sent_data(&copy);
        sleep(10);
    }

    close(sock);
    pthread_exit(NULL);
}
