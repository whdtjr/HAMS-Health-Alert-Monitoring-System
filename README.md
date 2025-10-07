### PPGData.h
운전자의 심장 박동이 감지될 때마다 생성되는 **PPG 원시 데이터와 HRV 도출 결과**를 하나의 구조체로 정의한 헤더 파일입니다.   

구조체(`PPGData`)에는 다음과 같은 항목이 포함됩니다:
- `timestamp` : 데이터 수집 시각
- `signal` : 원시 PPG 신호 값 (또는 필터링된 대표값)
- `bpm` : 분당 심박수 (Beats Per Minute)
- `ibi` : 박동 간 간격 (Inter-Beat Interval)
- `sdnn`, `rmssd`, `pnn50` : 주요 HRV 지표들  

이 구조체를 기반으로 실시간 심박 모니터링 및 졸음 감지 로직이 동작합니다.

### SharedData.h
모든 쓰레드에서 접근할 수 있는 최신 PPG 데이터를 전역 변수로 선언하고 이 데이터를 보호하기 위해 pthread_mutex_t 뮤텍스를 함께 정의한 헤더 파일입니다.

다른 소스 파일에서 이 헤더를 include하여 latest_data와 data_lock에 접근할 수 있습니다.

### main.c
PPG data 및 HRV 값들을 샘플링 및 도출하고 latest_data를 소켓을 이용해 다른 프로세스에게 전달하는 스레드를 여는 역할을 하는 코드가 정의된 파일입니다.

주요 global variable:
- `QS` : 박동이 감지되었음을 나타내는 플래그 (1이면 감지)

주요 function:
- `main`
  - 소켓 송신 스레드 (소켓을 이용해 latest_data를 외부 프로세스에게 보내는 스레드) 생성
  - startTimer()를 이용해 ppg data 및 HRV 값들 샘플링 시작
    - 현재 파일에서만 접근 가능한 전역 변수에 샘플링
  - QS가 1일 때 곧 심장 박동이 감지될때만 latest_data를 업데이트
    - latest_data는 모든 스레드에서 공유 가능
- `getPulse`
    - 타이머 시그널(SIGALRM)에 의해 주기적으로 호출되어 PPG 신호를 읽고 필터링하며 최신 10개의 IBI를 이용하여 BPM 및 HRV 지표를 산출하는 함수
    - 맥파의 피크(P)와 트로프(T)를 탐지해 심박을 감지하고 BPM 및 IBI를 계산한다.
    - 최근 10개의 IBI 값을 기반으로 SDNN, RMSSD, pNN50 등 HRV 지표를 산출한다.
    - 박동 감지 시 QS 플래그를 세워 메인 루프가 latest_data를 갱신할 수 있도록 한다.
- `sigHandler`
    - 프로그램 실행 도중 특정 시그널(SIGINT 등)이 발생하면 호출되는 시그널 핸들러 함수.
    - 타이머를 비활성화한 뒤 exit(EXIT_SUCCESS)를 호출하여 프로그램을 정상적으로 종료함.

