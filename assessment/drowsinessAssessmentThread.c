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


#include "BlockingQueue.h"
#include "PPGData.h"
#include "ClientInfo.h"
#include "SharedData.h"

int drowzyAlertCount = 0;
pthread_mutex_t drowzyAlertLock = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    double sumSdnn;
    double sumRmssd;
    double sumPnn50;
} HrvAccumulator;

void sumHrvValues(void* element, int index, void* ctx){
    PPGData* data = (PPGData*) element;
    HrvAccumulator* hrvAccumulator = (HrvAccumulator *) ctx;
    //printf("%d번째 데이터: sdnn-%lf, rmssd-%lf, pnn50-%lf\n", index,data ->sdnn, data->rmssd, data->pnn50 );
    hrvAccumulator -> sumSdnn += data ->sdnn;
    hrvAccumulator -> sumRmssd += data -> rmssd;
    hrvAccumulator -> sumPnn50 += data -> pnn50;
}

bool isFatiguedByHRV(BlockingQueue* buffer){
    int size = BlockingQueue_size(buffer);
    if(size == 0) return false;
    
    HrvAccumulator hrvAccumulator = {0, 0, 0};
    BlockingQueue_forEach(buffer, sumHrvValues, &hrvAccumulator);
    double avgOfSdnn = hrvAccumulator.sumSdnn / size;
    double avgOfRmssd = hrvAccumulator.sumRmssd / size;
    double avgOfPnn50 = hrvAccumulator.sumPnn50 / size;

    //sdnn이 30 미만 이거나 rmssd가 20 미만 이거나 pnn50이 0.25 미만
    if(avgOfSdnn < 30 || avgOfRmssd < 20 || avgOfPnn50 < 0.25) return true;
    return false;


    printf("졸음 감지 후 5분동안의 SDNN: %.2f, RMSSD: %.2f, PNN50: %.2f\n", avgOfSdnn, avgOfRmssd, avgOfPnn50);

    if(avgOfSdnn < 30 || avgOfRmssd < 20 || avgOfPnn50 < 0.25) {
        printf("→ 피로 상태입니다. (기준: SDNN < 30 || RMSSD < 20 || PNN50 < 0.25)\n");
        return true;
    } else {
        printf("→ 피로 상태가 아닙니다.\n");
        return false;
    }
}


void sendDrowsyModeMessage() {
   
    const char* fifo_path = "/tmp/assessmentt";
    int fd = open(fifo_path, O_WRONLY);
    if (fd != -1) {
        write(fd, "drowsiness\n", strlen("drowsiness\n"));
        close(fd);
    } else {
        perror("FIFO 열기 실패");
    }
}

//해당 스레드에 들어왔다는 건 캠이 졸음이라고 판단한 것이므로 바로 HRV 기반 피로 판단 함
void* drowsinessAssessmentThread(void* arg) {
    CLIENT_INFO * info = (CLIENT_INFO *) arg;
    printf("%s 스레드 시작\n", info -> id);
    // 이미 건강 상태 판단하는 스레드가 동작하고 있다면 현재 스레드는 바로 종료

    pthread_mutex_lock(&drowzyAlertLock);
    drowzyAlertCount++;
    int localCopy = drowzyAlertCount;
    pthread_mutex_unlock(&drowzyAlertLock);

    // HRV 기반 졸음 판단
    if(isFatiguedByHRV(ppgDataBuffer)){
        printf("졸음 감지 && 피로 판단 -> 졸음 모드 trigger\n");
        //피로하다고 판단시 보이스 서비스 프로세스에게 졸음 모드 보내기
        sendDrowsyModeMessage();
        pthread_mutex_lock(&drowzyAlertLock);
        drowzyAlertCount = 0;
        pthread_mutex_unlock(&drowzyAlertLock);
    }else if(localCopy >= 3){
       // HRV 기반 졸음 판단 로직에서 피로하다고 판단하진 않았으나 3번연속 졸음 판단 신호가 캠 AI한테서 옴
       // 보이스 서비스 프로세스에게 졸음 모드 보내고 연속갯수 판단 변수 0
       printf("졸음 감지 3번 발생 -> 졸음 모드 trigger\n");
       sendDrowsyModeMessage();
       pthread_mutex_lock(&drowzyAlertLock);
       drowzyAlertCount = 0;
       pthread_mutex_unlock(&drowzyAlertLock);
    }else{// HRV 기반 졸음 판단 로직도 안피로하다고 하고 졸음 판단 횟수도 3번 미만
        //아무 것도 안함
        printf("졸음 상태 아닌 것으로 판단\n");
    }
   
    // 자원 정리
    close(info->fd);
    free(info);
    pthread_mutex_lock(&drowsyStatus.lock);
    drowsyStatus.running = false;
    pthread_mutex_unlock(&drowsyStatus.lock);
    // 스레드 종료
    return NULL;

}