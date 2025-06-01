#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/stat.h>


#include "ClientInfo.h"
#include "SharedData.h"

void sendEmecyModeMessage() {
   
    const char* fifo_path = "/tmp/assessmentt";
    int fd = open(fifo_path, O_WRONLY);
    if (fd != -1) {
        write(fd, "emergency\n", strlen("emergency\n"));
        close(fd);
    } else {
        perror("FIFO 열기 실패");
    }
}
void* arrhythmiaAssessmentThread(void* arg) {
    CLIENT_INFO * info = (CLIENT_INFO *) arg;
    printf("%s 스레드 시작\n", info -> id);

    printf("건강 이상 판단 -> 응급 모드 trigger\n");
    //피로하다고 판단시 보이스 서비스 프로세스에게 졸음 모드 보내기
    sendEmecyModeMessage();

    close(info->fd);
    free(info);
    pthread_mutex_lock(&arrhythStatus.lock);
    arrhythStatus.running = false;
    pthread_mutex_unlock(&arrhythStatus.lock);
    return NULL;
}