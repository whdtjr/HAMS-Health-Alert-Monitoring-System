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

#define BUF_SIZE 512

void error_handling(const char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}


void print_ppg_data(void* element) {
    PPGData* data = (PPGData*) element;
    char* timestamp_str = ctime(&data->timestamp);
    if (timestamp_str[strlen(timestamp_str)-1] == '\n') timestamp_str[strlen(timestamp_str)-1] = '\0';

    printf("Time: %s | Signal: %d | BPM: %d | IBI: %d | SDNN: %.2lf | RMSSD: %.2lf | PNN50: %.2lf",
            timestamp_str, data->signal, data->bpm, data->ibi, data->sdnn, data->rmssd, data->pnn50);
}


int main(int argc, char **argv){

    //1. Blocking Queue 큐 준비 (max size는 6)
    BlockingQueue* bqueue = new_BlockingQueue(6);
   
    //2. 소켓 set up
    //port 번호가 제공되었는지 확인
    if(argc!=2){
        printf("Usage : %s <port>\n",argv[0]);
    }

    //socket 생성 및 에러 핸들링
    int serv_sock,clnt_sock=-1;
    struct sockaddr_in serv_addr,clnt_addr;
    socklen_t clnt_addr_size;
    char msg[2];
    
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if(serv_sock == -1)
        error_handling("socket() error");
        
    memset(&serv_addr, 0 , sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));
    
    if(bind(serv_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr))==-1)
        error_handling("bind() error");
        
    if(listen(serv_sock,5) == -1)
            error_handling("listen() error");

    printf("클라이언트 연결을 기다리는 중..\n");

        if(clnt_sock<0){           
                clnt_addr_size = sizeof(clnt_addr);
                clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr,     &clnt_addr_size);
                if(clnt_sock == -1)
                    error_handling("accept() error");   
                else
                    printf("클라이언트 연결 성공: %s:%d\n", inet_ntoa(clnt_addr.sin_addr), ntohs(clnt_addr.sin_port));
        }
    
    char buffer[BUF_SIZE];
    //3. 시스템이 살아있는 동안 아래내용 반복    
    while(1) 
    {
        memset(buffer, 0, BUF_SIZE);
        printf("initial buffer: %s\n", buffer);
        //TCP 소켓으로부터 데이터가 도착할 때까지 block 상태로 대기
        int str_len = recv(clnt_sock, buffer, BUF_SIZE - 1, 0);
        if (str_len <= 0) break;

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
        BlockingQueue_enq_with_overwrite(bqueue, data);

        //4.디버깅 용 로그 출력
        printf("---- Queue Contents ----\n");
        BlockingQueue_print(bqueue, print_ppg_data);
        printf("------------------------\n");
    }

    //4. 자원정리
    BlockingQueue_clear(bqueue);
    BlockingQueue_destroy(bqueue);
    close(clnt_sock);
    close(serv_sock);

     
    return(0);
}

