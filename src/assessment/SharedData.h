// SharedData.h
#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#include "BlockingQueue.h"

typedef struct {
    bool running;
    pthread_mutex_t lock;
} ThreadStatus;

extern BlockingQueue* ppgDataBuffer;// 모든 쓰레드에서 공유할 PPGData 버퍼
extern int drowzyAlertCount;
extern ThreadStatus hrvStatus;
extern ThreadStatus drowsyStatus;
extern ThreadStatus arrhythStatus;

#endif // SHARED_DATA_H
