// SharedData.h
#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#include <pthread.h>
#include "PPGData.h"  // PPGData 구조체 정의 포함

// 모든 쓰레드에서 공유할 최신 PPG 데이터
extern PPGData latest_data;

// 쓰레드 안전을 위한 mutex
extern pthread_mutex_t data_lock;

#endif // SHARED_DATA_H
