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
#include "ThreadEntry.h"
#include "SharedData.h"

#define BUF_SIZE 512
#define PPG_BUF_SIZE 30

ThreadStatus hrvStatus = {false, PTHREAD_MUTEX_INITIALIZER};
ThreadStatus drowsyStatus = {false, PTHREAD_MUTEX_INITIALIZER};
ThreadStatus arrhythStatus = {false, PTHREAD_MUTEX_INITIALIZER};
BlockingQueue* ppgDataBuffer;

void error_handling(const char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

bool try_start_thread(ThreadStatus* status, void* (*entry)(void*), CLIENT_INFO* info) {
    pthread_t tid;
    pthread_mutex_lock(&status->lock);
    if (!status->running) {
        status->running = true;
        pthread_mutex_unlock(&status->lock);
        pthread_create(&tid, NULL, entry, info); 
        pthread_detach(tid);
        return true;
    } else {
        pthread_mutex_unlock(&status->lock);
        close(info->fd);
        free(info);
        return false;
    }
}

int main(int argc, char **argv){
    ppgDataBuffer = new_BlockingQueue(PPG_BUF_SIZE);

    char buffer[BUF_SIZE];
    char clientId[ID_SIZE];

    //port 번호가 제공되었는지 확인
    if(argc!=2){
        printf("Usage : %s <port>\n",argv[0]);
    }

    //socket 생성 및 에러 핸들링
    int serv_sock,clnt_sock=-1;
    struct sockaddr_in serv_addr,clnt_addr;
    socklen_t clnt_addr_size;
    
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

    //시스템이 살아있는 동안 아래내용 반복    
    while(1) 
    {
            
        clnt_addr_size = sizeof(clnt_addr);
        clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);

        if(clnt_sock == -1)
            error_handling("accept() error");   
        else
            printf("클라이언트 연결 성공: %s:%d\n", inet_ntoa(clnt_addr.sin_addr), ntohs(clnt_addr.sin_port));
        

        int str_len = recv(clnt_sock, buffer, BUF_SIZE - 1, 0);
   
        cJSON* root = cJSON_Parse(buffer);
        if (!root) {
            fprintf(stderr, "JSON 파싱 실패\n");
            close(clnt_sock);
            break;
        }

        cJSON* client_item = cJSON_GetObjectItem(root, "id");
        if (client_item && cJSON_IsString(client_item)) {
            strcpy(clientId, client_item->valuestring);
            printf("받은 id: %s\n", clientId);
        } else {
            fprintf(stderr, "status 항목 없음 또는 문자열 아님\n");
        }

        cJSON_Delete(root);

        CLIENT_INFO* info = malloc(sizeof(CLIENT_INFO));
        info -> fd = clnt_sock;
        strcpy(info -> ip, inet_ntoa(clnt_addr.sin_addr));
        strcpy(info -> id, clientId);

        if (strcmp(clientId, "hrv") == 0) {
            if (try_start_thread(&hrvStatus, hrvReceiverThread, info)) {
                printf("[INFO] %s 스레드 시작\n", clientId);
            } else {
                printf("[INFO] %s 스레드는 이미 실행 중\n", clientId);
            }
        }else if(strcmp(clientId, "drowsy") == 0){
            if (try_start_thread(&drowsyStatus, drowsinessAssessmentThread, info)) {
                printf("[INFO] %s 스레드 시작\n", clientId);
            } else {
                printf("[INFO] %s 스레드는 이미 실행 중\n", clientId);
            }
        }else{
            if (try_start_thread(&arrhythStatus, arrhythmiaAssessmentThread, info)) {
                printf("[INFO] %s 스레드 시작\n", clientId);
            } else {
                printf("[INFO] %s 스레드는 이미 실행 중\n", clientId);
            }
        }
    }

    //자원정리
    close(clnt_sock);
    close(serv_sock);
     
    return(0);
}

