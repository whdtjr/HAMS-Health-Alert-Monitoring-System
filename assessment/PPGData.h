// PPHData.h
#ifndef PPGDATA_H
#define PPGDATA_H

#include <stdbool.h>
#include <time.h> 

typedef struct {
    time_t timestamp;  // 데이터 수집 시각 
    int signal;        // 원시 시그널 값 또는 필터링된 대표값
    int bpm;           // 분당 심박수 (Beats Per Minute)
    int ibi;           // 심박수간 간격
    double sdnn;       // 표준 편차 (HRV 지표)
    double rmssd;      // Root Mean Square of Successive Differences
    double pnn50;      // 50ms 넘는 IBI 비율 (HRV 지표)
} PPGData;
#endif // PPGDATA_H
