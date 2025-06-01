#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>
#include <cjson/cJSON.h>

#include "BlockingQueue.h"
#include "PPGData.h"
#include "ClientInfo.h"
#include "SharedData.h"

#define BUF_SIZE 512


void print_ppg_data(void* element) {
    PPGData* data = (PPGData*) element;
    char* timestamp_str = ctime(&data->timestamp);
    if (timestamp_str[strlen(timestamp_str)-1] == '\n') timestamp_str[strlen(timestamp_str)-1] = '\0';

    printf("Time: %s | Signal: %d | BPM: %d | IBI: %d | SDNN: %.2lf | RMSSD: %.2lf | PNN50: %.2lf",
            timestamp_str, data->signal, data->bpm, data->ibi, data->sdnn, data->rmssd, data->pnn50);
}


void* hrvReceiverThread(void* arg) {

    CLIENT_INFO * info = (CLIENT_INFO *) arg;

  
    printf("%s 스레드 시작\n", info -> id);
    
    char buffer[BUF_SIZE];
    while(1) 
    {
        memset(buffer, 0, BUF_SIZE);
        //TCP 소켓으로부터 데이터가 도착할 때까지 block 상태로 대기
        int str_len = recv(info -> fd, buffer, BUF_SIZE - 1, 0);
        if (str_len <= 0) break; //수정

        // JSON 파싱
        cJSON* root = cJSON_Parse(buffer);
        if (!root) continue;

        PPGData* data = malloc(sizeof(PPGData));
        data->signal = cJSON_GetObjectItem(root, "signal")->valueint;
        data->bpm = cJSON_GetObjectItem(root, "bpm")->valueint;
        data->ibi = cJSON_GetObjectItem(root, "ibi")->valueint;
        data->sdnn = cJSON_GetObjectItem(root, "sdnn")->valuedouble;
        data->rmssd = cJSON_GetObjectItem(root, "rmssd")->valuedouble;
        data->pnn50 = cJSON_GetObjectItem(root, "pnn50")->valuedouble;
        data->timestamp = (time_t)cJSON_GetObjectItem(root, "timestamp")->valuedouble;

        cJSON_Delete(root);

        // 3. Blocking Queue에 enqueue (오버라이트 방식)
        BlockingQueue_enq_with_overwrite(ppgDataBuffer, data);

        //4.디버깅 용 로그 출력
        printf("---- Queue Contents ----\n");
        BlockingQueue_print(ppgDataBuffer, print_ppg_data);
        printf("------------------------\n");
    }

    //4. 자원정리
    BlockingQueue_clear(ppgDataBuffer); //실제 PPGData들 다 clear 되는지 확인
    BlockingQueue_destroy(ppgDataBuffer);

    close(info->fd);
    free(info);
    return NULL;
}

