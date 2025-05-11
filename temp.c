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

#define ID_SIZE 10

typedef struct {
    int fd;
    char ip[20];
    char id[ID_SIZE];

}CLIENT_INFO;

void* hrvReceiverThread(void* arg) {
    CLIENT_INFO * info = (CLIENT_INFO *) arg;
    printf("%s 스레드 시작\n", info -> id);
    close(info->fd);
    free(info);
    return NULL;
 }
void* drowsinessAssessmentThread(void* arg) {
    CLIENT_INFO * info = (CLIENT_INFO *) arg;
    printf("%s 스레드 시작\n", info -> id);
    printf("소켓 fd: %d\n", info -> fd);
    char buffer[BUF_SIZE];

    int str_len = recv(info -> fd, buffer, BUF_SIZE - 1, 0);
    if (str_len <= 0) {
        close(info->fd);
        free(info);
        return NULL;
    }
    

    // JSON 파싱
    cJSON* root = cJSON_Parse(buffer);
    if (!root) {
        fprintf(stderr, "JSON 파싱 실패\n");
        close(info->fd);
        free(info);
        return NULL;
    }

    char status[10];
    cJSON* status_item = cJSON_GetObjectItem(root, "status");
    if (status_item && cJSON_IsString(status_item)) {
        strcpy(status, status_item->valuestring);
        printf("받은 status: %s\n", status);
    } else {
        fprintf(stderr, "status 항목 없음 또는 문자열 아님\n");
    }

    cJSON_Delete(root);
    close(info->fd);
    free(info);
    return NULL;

}
void* arrhythmiaAssessmentThread(void* arg) {
    CLIENT_INFO * info = (CLIENT_INFO *) arg;
    printf("%s 스레드 시작\n", info -> id);
    close(info->fd);
    free(info);
    return NULL;
}

void error_handling(const char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv){

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
        

        int	str_len = read(clnt_sock, clientId, sizeof(clientId));
        clientId[str_len] = '\0';

        CLIENT_INFO* info = malloc(sizeof(CLIENT_INFO));
        info -> fd = clnt_sock;
        strcpy(info -> ip, inet_ntoa(clnt_addr.sin_addr));
        strcpy(info -> id, clientId);
        pthread_t tid;

        if (strcmp(clientId, "hrv") == 0) {
            pthread_create(&tid, NULL, hrvReceiverThread, info); 
        }else if(strcmp(clientId, "drowzy") == 0){
            pthread_create(&tid, NULL, drowsinessAssessmentThread, info); 
        }else{
            pthread_create(&tid, NULL, arrhythmiaAssessmentThread, info); 
        }
        pthread_detach(tid);
        
    }

    //4. 자원정리
    close(clnt_sock);
    close(serv_sock);
     
    return(0);
}

