#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include "ThreadEntry.h"
#include "SharedData.h"
 
#define GPS_DEVICE "/dev/serial0" //나중에 수정 serial0
#define BUFFER_SIZE 256

int debug = 1;

// NMEA 포맷에서 위도/경도를 십진수 형태로 변환
double convertToDecimalDegrees(char* nmeaCoord, char quadrant) {
    double raw = atof(nmeaCoord);
    int degrees = (int)(raw / 100);
    double minutes = raw - (degrees * 100);
    double decimal = degrees + (minutes / 60.0);
    if (quadrant == 'S' || quadrant == 'W') decimal *= -1;
    return decimal;
}

void * gpsReadThread(void * arg){
    printf("Gps Read Thread Start\n");
    int serial_port = open(GPS_DEVICE, O_RDONLY | O_NOCTTY);
    if (serial_port < 0) {  
        perror("Serial port open error");
         pthread_exit(NULL); 
    }

    struct termios tty;
    memset(&tty, 0, sizeof(tty));
    tcgetattr(serial_port, &tty);

    // Baud rate 설정 
    cfsetispeed(&tty, B9600);
    cfsetospeed(&tty, B9600);

    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;


    tcsetattr(serial_port, TCSANOW, &tty);
    char buffer[BUFFER_SIZE];

    while (1) {
        int n = read(serial_port, buffer, sizeof(buffer) - 1);
        if (n > 0) {
            buffer[n] = '\0';

            // 예: $GPRMC,092751.000,A,5321.6802,N,00630.3372,W,0.06,31.66,280511,,,A*45
            if (strstr(buffer, "$GPRMC")) {
                char* token = strtok(buffer, ",");
                // printf("first token: %s\n", token);
                int field = 0;
                char lat[16], lng[16], ns = 'N', ew = 'E';

                while (token != NULL) {
                    field++;
                    if (field == 4) strncpy(lat, token, sizeof(lat));
                    if (field == 5) ns = token[0];
                    if (field == 6) strncpy(lng, token, sizeof(lng));
                    if (field == 7) ew = token[0];
                    token = strtok(NULL, ",");
                }

                if (strlen(lat) > 0 && strlen(lng) > 0) {
                    double latitude = convertToDecimalDegrees(lat, ns);
                    double longitude = convertToDecimalDegrees(lng, ew);
                    printf("Latitude: %.6f, Longitude: %.6f\n", latitude, longitude);
                    if(debug){
                        if(latitude == 0.0 || longitude == 0.0){ //디버깅용
                            latitude = 37.2778;
                            longitude = 127.0426;
                            printf("Latitude: %.4f, Longitude: %.4f\n", latitude, longitude);
                        }
                    }
            
                    //공유 자원에 저장
                    pthread_mutex_lock(&locationLock);
                    curLocation.lat = latitude;
                    curLocation.lng = longitude;
                    pthread_mutex_unlock(&locationLock);
                }
            }
        }
        usleep(500000); // 0.5초 대기
    }
    close(serial_port);
    pthread_exit(NULL); 

 }
