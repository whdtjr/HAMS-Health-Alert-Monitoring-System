/*

		THIS CODE IS RELEASED WITHOUT WARRANTY OF FITNESS
		OR ANY PROMISE THAT IT WORKS, EVEN. WYSIWYG.

		YOU SHOULD HAVE RECEIVED A LICENSE FROM THE MAIN
		BRANCH OF THIS REPO. IF NOT, IT IS USING THE
		MIT FLAVOR OF LICENSE

*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdarg.h>
#include <signal.h>
#include <time.h>
#include <math.h>
#include <pthread.h>

#include "spi_reader.h"
#include "SharedData.h"
#include "sendFeature.h"
// #include <wiringPi.h>
// #include <mcp3004.h>


#define OPT_R 10        // min uS allowed lag btw alarm and callback
#define OPT_U 2000      // sample time uS between alarms
//2000uS 마다 샘플링 하려는 듯 샘플링 간격: 2mS -> 0.002초 마다 한번씩 샘플링 곧 500Hz
#define OPT_O_ELAPSED 0 // output option uS elapsed time between alarms
#define OPT_O_JITTER 1  // output option uS jitter (elapsed time - sample time)
#define OPT_O 1         // defaoult output option
#define OPT_C 10000     // number of samples to run (testing)
#define OPT_N 1         // number of Pulse Sensors (only 1 supported)

#define TIME_OUT 30000000    // uS time allowed without callback response (30초)
// PULSE SENSOR LEDS
#define BLINK_LED 0
// MCP3004/8 SETTINGS
#define BASE 100
#define SPI_CHAN 0

// FIFO STUFF
#define PULSE_EXIT 0    // CLEAN UP AND SHUT DOWN
#define PULSE_IDLE 1    // STOP SAMPLING, STAND BY
#define PULSE_ON 2      // START SAMPLING, WRITE DATA TO FILE
#define PULSE_DATA 3    // SEND DATA PACKET TO FIFO
#define PULSE_CONNECT 9 // CONNECT TO OTHER END OF PIPE


#define MA_WINDOW 5
int signalBuffer[MA_WINDOW];
int signalIndex = 0;


// VARIABLES USED TO DETERMINE SAMPLE JITTER & TIME OUT
volatile unsigned long eventCounter, thisTime, lastTime, elapsedTime, jitter;
volatile int sampleFlag = 0; //getPulse()가 실행되었는지 표시하는 플래그
volatile int sumJitter, firstTime, secondTime, duration;
unsigned long timeOutStart, dataRequestStart, m; 
//timeOutStart : 마지막 유효 샘플링 시작 시간
// VARIABLES USED TO DETERMINE BPM
volatile int Signal; //현재 아날로그 센서로부터 읽은 신호값
volatile unsigned int sampleCounter;
volatile int threshSetting,lastBeatTime; 
volatile int thresh = 550;
volatile int P = 512;                               // set P default
volatile int T = 512;                               // set T default
volatile int firstBeat = 1;                      // set these to avoid noise
volatile int secondBeat = 0;                    // when we get the heartbeat back
volatile int QS = 0; //박동이 감지되었음을 나타내는 플래그
volatile int rate[10]; //최근 10개의 IBI 값 (평균 내기 위해 사용)
volatile int BPM = 0; 
volatile int IBI = 600;                  // 600ms per beat = 100 Beats Per Minute (BPM)
///새로 추가 수정/////
volatile double SDNN = 0;
volatile double RMSSD = 0;
volatile double PNN50 = 0;
///새로 추가 수정/////
volatile int Pulse = 0; //현재 박동 감지 상태 (0: 없음, 1: 감지됨)
volatile int amp = 100;                  // beat amplitude 1/10 of input range.
// LED CONTROL
volatile int fadeLevel = 0;
// FILE STUFF
char filename [100];
struct tm *timenow;

PPGData latest_data;
pthread_mutex_t data_lock = PTHREAD_MUTEX_INITIALIZER;

//named_pipe fd
int pipe_fd = -1;

// FUNCTION PROTOTYPES
void getPulse(int sig_num);
void startTimer(int r, unsigned int u);
void stopTimer(void);
void initPulseSensorVariables(void);
void initJitterVariables(void);
double getSdnn(int* IBIList, int IBIAvg, int N);
double getRmssd(int* IBIList, int N);
double getPnn50(int* IBIList, int N);


FILE *data;

//시스템 부팅 후 경과한 시간을 마이크로 초 단위로 return하는 함수
unsigned long micros() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts); // 시스템의 단조 시간 (절대 시간이 아님)
  return (unsigned long)(ts.tv_sec * 1000000 + ts.tv_nsec / 1000); //마이크로세컨드 단위로 변환
}


//프로그램 실행 방법과 출력 데이터의 저장 위치 및 형식을 설명하는 안내 함수
void usage()
{
   fprintf
   (stderr,
      "\n" \
      "Usage: sudo ./pulseProto ... [OPTION] ...\n" \
      "   NO OPTIONS AVAILABLE YET\n"\
      "\n"\
      "   Data file saved as\n"\
      "   /home/pi/Documents/PulseSensor/PULSE_DATA <timestamp>\n"\
      "   Data format tab separated:\n"\
      "     sampleCount  Signal  BPM  IBI  Pulse  Jitter\n"\
      "\n"
   );
}

// 프로그램 실행 중 특정 시그널(Signal)이 발생했을 때 실행되는 시그널 핸들러 함수를 직접 정의한 것. 
// 주로 프로그램을 안전하게 종료할 때 사용
void sigHandler(int sig_num){
	printf("\nkilling timer\n");
  startTimer(OPT_R,0); // kill the alarm
  //타이머를 0 마이크로초로 설정, 즉 알람을 비활성화
	exit(EXIT_SUCCESS); //정상적으로 프로그램 종료
}

// 에러 발생 시 프로그램을 종료하기 위한 공용 함수
void fatal(int show_usage, char *fmt, ...)
{
  //fmt -> printf 스타일 포맷 문자열. (formatter인듯)
  //에러 메시지 포맷팅
   char buf[128];
   va_list ap;
   char kill[20];

   va_start(ap, fmt);
   vsnprintf(buf, sizeof(buf), fmt, ap);
   va_end(ap);

   //에러 메시지 출력
   fprintf(stderr, "%s\n", buf);

   //show_usage: 1이면 사용법(도움말)을 출력.
   if (show_usage) usage();

   //출력 버퍼를 비우기
   fflush(stderr);
   //타이머 종료 메시지를 출력
   printf("killing timer\n");
   startTimer(OPT_R,0); // kill the alarm
   //10 마이크로초 후 부터 SIGARM 발생 안하게 됨
   //에러 메시지를 data 파일에도 기록
   fprintf(data,"#%s",fmt);
   //파일 스트림 종료
   fclose(data);
   //프로그램 종료
   exit(EXIT_FAILURE);
}

// SAVED FOR FUTURE FEATURES
static int initOpts(int argc, char *argv[])
{
   //int i, opt;
   //while ((opt = getopt(argc, argv, ":")) != -1)
   //{
      //i = -1;
      //switch (opt)
      //{
        //case '':
        //default: /* '?' */
           //usage();
        //}
    //}
   return optind;
}


int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_ip> <server_port>\n", argv[0]);
        exit(1);
    }

    printf("Connecting to %s:%s\n", argv[1], argv[2]);

    char* send_args[2] = { argv[1], argv[2] };
    pthread_t send_thread;
    if (pthread_create(&send_thread, NULL, send_feature_thread, send_args) != 0) {
        perror("pthread_create failed");
        exit(1);
    }
    
    // 초기화 함수에서 파이프 열기 (예: setup() 또는 main 루프 시작 시)
    pipe_fd = open("/tmp/signal_pipe", O_WRONLY);
    if (pipe_fd == -1) {
        perror("Failed to open FIFO");
        // 적절한 에러 처리
    }

    //SIGINT(2) == Ctrl + C 발생시 실행될 콜백 등록 등록
    signal(SIGINT,sigHandler);
    //int settings = 0;
    // command line settings
    //settings = initOpts(argc, argv);
    //파일 이름 생성 및 로그 파일 오픈
    time_t now = time(NULL);
    timenow = gmtime(&now);

    strftime(filename, sizeof(filename),
    "/home/pi/Documents/PulseSensor/PULSE_DATA_%Y-%m-%d_%H:%M:%S.dat", timenow);
    //파일을 쓰기 모드로 열고 로그 기록 준비
    data = fopen(filename, "w+");
    fprintf(data,"#Running with %d latency at %duS sample rate\n",OPT_R,OPT_U);
    fprintf(data,"#sampleCount\tSignal\tBPM\tIBI\tjitter\n");

    printf("Ready to run with %d latency at %duS sample rate\n",OPT_R,OPT_U);

    if(init(0) == -1){
      fatal(0,"spi init fail",0);
    }


    initPulseSensorVariables();  // initilaize Pulse Sensor beat finder  BPM 측정 관련 변수 초기화

    startTimer(OPT_R, OPT_U);   // start sampling
    // 타이머 설정: OPT_R 신호로 OPT_U 간격 마다 getPulse 실행

    while(1)
    {
        if (QS == 1) {
            QS = 0; // 다시 꺼줘야 다음 박동에서만 다시 1됨
            timeOutStart = micros();
    
            // HRV time-domain 값들 여기서 추가 가능
            printf("thresh: %d | sample: %lu ms | Signal: %d | BPM: %d | IBI: %d ms | SDNN: %.2lf ms | RMMSSD: %.2lf ms | PNN50: %.2lf\n",
                   thresh, sampleCounter, Signal, BPM, IBI, SDNN, RMSSD, PNN50);

            pthread_mutex_lock(&data_lock);
            latest_data.timestamp = time(NULL);  // 수집 시각
            latest_data.bpm = BPM;
            latest_data.ibi = IBI;
            latest_data.signal = Signal;
            latest_data.sdnn = SDNN;
            latest_data.rmssd = RMSSD;
            latest_data.pnn50 = PNN50;
        
            pthread_mutex_unlock(&data_lock);

    
            // fprintf(data, "%d\t%d\t%d\t%d\t%d\t%d\n",
            //         sampleCounter, Signal, IBI, BPM, jitter, duration);
        }
    
         if((micros() - timeOutStart)>TIME_OUT){
            fatal(0,"0-program timed out",0);
         }
    }

    pthread_join(send_thread, NULL);
    return 0;

}

// 타이머를 설정하고, 주기적으로 SIGALRM 시그널을 발생시키도록 설정. 
// 센서 데이터를 주기적으로 읽도록 트리거하는 데 사용
void startTimer(int r, unsigned int u){
    int latency = r; //처음 알람 발생까지의 지연 시간 (마이크로초)
    unsigned int micros = u; // 반복 주기 (마이크로초)

    //SIGALRM(14) == 알람 타이머 시그널
    //알람 타이머 시그널이 발생하면 getPulse 함수 실행하도록 콜백 등록
    signal(SIGALRM, getPulse);
    //ualarm: 마이크로초 단위로 알람을 설정하는 함수
    int err = ualarm(latency, micros);
    if(err == 0){ //기존에 설정된 알람이 없었다면
        //반복 여부에 따라 메시지 출력
        if(micros > 0){
            printf("ualarm ON\n");
        }else{
            printf("ualarm OFF\n");
        }
    }

}

// 심박 센서 분석에 필요한 모든 변수들을 초기화
// 프로그램 시작 시 또는 리셋이 필요할 때 
// 이 함수를 호출해서 시그널 분석 상태를 처음 상태로 되돌리는 것이 목적
void initPulseSensorVariables(void){
   //심박수 계산을 위한 최근 10개의 박동 간 간격(IBI) 저장 배열을 초기화
    for (int i = 0; i < 10; ++i) {
        rate[i] = 0;
    }
    QS = 0; //새 심박이 감지되었는지 여부 (Pulse가 새로 측정되면 1)
    BPM = 0; //분당 심박수 (Beats Per Minute)
    IBI = 600;                  // 600ms per beat = 100 Beats Per Minute (BPM)
    SDNN = 0;
    RMSSD = 0;
    PNN50 = 0;
    //박동 간 간격(Inter-Beat Interval), 초기값 600ms
    Pulse = 0; //현재 심박이 감지 중인지 여부
    sampleCounter = 0; //샘플 카운터 (시간 누적용)
    lastBeatTime = 0; //마지막 심박이 감지된 시간
    P = 512;                    // peak at 1/2 the input range of 0..1023
    //신호의 peak(P), 초기값은 중간값 (0~1023 범위에서)
    T = 512;                    // trough at 1/2 the input range.
    // trough(T),
    threshSetting = 550;        // used to seed and reset the thresh variable
    //심박 감지 임계값 설정 및 현재값
    thresh = 550;     // threshold a little above the trough
    amp = 100;                  // beat amplitude 1/10 of input range.
    //심박 파형의 진폭 (P-T 차이)
    firstBeat = 1;           // looking for the first beat 첫 번째 심박을 찾는 중
    secondBeat = 0;         // not yet looking for the second beat in a row 두 번째 심박 찾기 전
    lastTime = micros(); //현재 시간 저장 (마이크로초 단위)
    timeOutStart = lastTime; //타임아웃 측정을 위한 기준 시간 저장
}

int getSmoothedSignal(int newValue) {
    signalBuffer[signalIndex] = newValue;
    signalIndex = (signalIndex + 1) % MA_WINDOW;

    int sum = 0;
    for (int i = 0; i < MA_WINDOW; i++) {
        sum += signalBuffer[i];
    }
    return sum / MA_WINDOW;
}


//심박(Pulse, 즉 심장 박동)을 측정 (BPM 계산)
void getPulse(int sig_num){

    if(sig_num == SIGALRM)
    {
      thisTime = micros();
     //Signal = analogRead(BASE);
      //Signal = readValue();
      Signal = getSmoothedSignal(readValue());

      if(Signal == -1){
        fatal(0,"spi not yet initialize",0);
      }
      if(pipe_fd != -1)
      {
        int signalValue = Signal;
        write(pipe_fd, &signalValue, sizeof(int));
      }

      elapsedTime = thisTime - lastTime;
      lastTime = thisTime;
      jitter = elapsedTime - OPT_U;
      sumJitter += jitter;
      sampleFlag = 1;


      sampleCounter += 2;         // keep track of the time in mS with this variable
      //매 2ms 주기로 호출된다고 가정하고 시간 누적
      int N = sampleCounter - lastBeatTime;      // monitor the time since the last beat to avoid noise

      // FADE LED HERE, IF WE COULD FADE...

      //  find the peak and trough of the pulse wave
      if (Signal < thresh && N > (IBI / 5) * 3) { // avoid dichrotic noise by waiting 3/5 of last IBI
        if (Signal < T) {                        // T is the trough
          T = Signal;                            // keep track of lowest point in pulse wave
        }
      }

      if (Signal > thresh && Signal > P) {       // thresh condition helps avoid noise
        P = Signal;                              // P is the peak
      }                                          // keep track of highest point in pulse wave

      //  NOW IT'S TIME TO LOOK FOR THE HEART BEAT
      // signal surges up in value every time there is a pulse
      if (N > 300) {                             // avoid high frequency noise
        if ( (Signal > thresh) && (Pulse == 0) && (N > ((IBI / 5) * 3)) ) {
          Pulse = 1;                             // set the Pulse flag when we think there is a pulse
          IBI = sampleCounter - lastBeatTime;    // measure time between beats in mS
          lastBeatTime = sampleCounter;          // keep track of time for next pulse

          //처음 두 박동은 정확하지 않기 때문에 따로 처리
          if (secondBeat) {                      // if this is the second beat, if secondBeat == TRUE
            secondBeat = 0;                      // clear secondBeat flag
            for (int i = 0; i <= 9; i++) {       // seed the running total to get a realisitic BPM at startup
              rate[i] = IBI;
            }
          }
          if (firstBeat) {                       // if it's the first time we found a beat, if firstBeat == TRUE
            firstBeat = 0;                       // clear firstBeat flag
            secondBeat = 1;                      // set the second beat flag
            // IBI value is unreliable so discard it
            return;
          }


          // keep a running total of the last 10 IBI values
          int runningTotal = 0;                  // clear the runningTotal variable

          for (int i = 0; i <= 8; i++) {          // shift data in the rate array
            rate[i] = rate[i + 1];                // and drop the oldest IBI value
            runningTotal += rate[i];              // add up the 9 oldest IBI values
          }
          if (abs(IBI - rate[9]) <= 300) {// 갑작스러운 IBI 변화 거르기
              rate[9] = IBI;                          // add the latest IBI to the rate array
              runningTotal += rate[9];                // add the latest IBI to runningTotal
              runningTotal /= 10;                     // average the last 10 IBI values
              BPM = 60000 / runningTotal;             // how many beats can fit into a minute? that's BPM!
              SDNN = getSdnn((int*)rate, runningTotal, 10);
              RMSSD = getRmssd((int*)rate, 10);
              PNN50 = getPnn50((int*)rate, 10);
              QS = 1;                              // set Quantified Self flag (we detected a beat)

          }
          
        }
      }

      //신호가 내려가면 (즉, 박동 종료 시점), 다음 박동을 위한 임계값 갱신
      if (Signal < thresh && Pulse == 1) {  // when the values are going down, the beat is over
        Pulse = 0;                         // reset the Pulse flag so we can do it again
        amp = P - T;                           // get amplitude of the pulse wave
        thresh = amp / 2 + T;                  // set thresh at 50% of the amplitude
        P = thresh;                            // reset these for next time
        T = thresh;
        printf("threshold: %d\n", thresh);
      }

      //비정상 (무박동) 처리
      if (N > 2500) {  
                                // if 2.5 seconds go by without a beat  2.5초간 박동 없으면 초기화
        thresh = threshSetting;                // set thresh default
        P = 512;                               // set P default
        T = 512;                               // set T default
        lastBeatTime = sampleCounter;          // bring the lastBeatTime up to date
        firstBeat = 1;                      // set these to avoid noise
        secondBeat = 0;                    // when we get the heartbeat back
        QS = 0;
        BPM = 0;
        IBI = 600;                  // 600ms per beat = 100 Beats Per Minute (BPM)
        SDNN = 0;
        RMSSD =0;
        PNN50 = 0;
        Pulse = 0;
        amp = 100;                  // beat amplitude 1/10 of input range.
      }

      //이 함수가 실행되는 데 걸린 시간 측정 (디버깅용)
      duration = micros()-thisTime;

    }

}
double getSdnn(int* IBIList, int IBIAvg, int N){
    //IBIList: 최신 N개의 IBI 값 배열
    //슬라이딩 윈도우 기법으로 밖에서 만들고 옴
    //IBIAvg: 최신 N개의 IBI 값 평균 -> 
    //물론 여기서 구할수도 있지만 슬라이딩 윈도우 기법으로 밖에서 구하고 온다고 가정
    double sdnn = 0;
    int sum = 0;
    for(int i=0; i<N; i++){
        int diff = IBIAvg - IBIList[i];
        sum += diff * diff;
    }
    sdnn = sqrt( ((double) sum) / N);

    return sdnn;
}

double getRmssd(int* IBIList, int N){
   double rmssd = 0;
   int sum = 0;
   for(int i = 1; i<N;i++){
    int sdiff = IBIList[i] - IBIList[i-1];
    sum += sdiff * sdiff;
   }
   rmssd = sqrt((double)sum / (N-1));
   return rmssd;
}

double getPnn50(int* IBIList, int N){
    double pnn50 = 0;
    int count = 0;
    for(int i =1; i<N; i++){
        int sdiff =abs(IBIList[i] - IBIList[i - 1]);
        if(sdiff > 50) count ++;
    }
    return ((double) count) / (N - 1);

 }