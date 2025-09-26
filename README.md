### PPGData.h
운전자의 심장 박동이 감지될 때마다 생성되는 **PPG 원시 데이터와 HRV 도출 결과**를 하나의 구조체로 정의한 헤더 파일입니다.   

구조체(`PPGData`)에는 다음과 같은 항목이 포함됩니다:
- `timestamp` : 데이터 수집 시각
- `signal` : 원시 PPG 신호 값 (또는 필터링된 대표값)
- `bpm` : 분당 심박수 (Beats Per Minute)
- `ibi` : 박동 간 간격 (Inter-Beat Interval)
- `sdnn`, `rmssd`, `pnn50` : 주요 HRV 지표들  

이 구조체를 기반으로 실시간 심박 모니터링 및 졸음 감지 로직이 동작합니다.
