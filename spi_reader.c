#include <getopt.h>
#include <linux/spi/spidev.h>
#include <linux/types.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>
#include "spi_reader.h"


#define IN 0
#define OUT 1
#define LOW 0
#define HIGH 1
#define VALUE_MAX 256
#define BUFFER_MAX 3
#define DIRECTION_MAX 45

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

static const char *DEVICE = "/dev/spidev0.0";
static uint8_t MODE = 0; //SPI 모드 설정 (예: CPOL/CPHA)
static uint8_t BITS = 8; //데이터 전송 비트 수 (8비트)
static uint32_t CLOCK = 1000000; //SPI 클럭 속도 (1MHz)
static uint16_t DELAY = 5; //전송 사이 지연 (5μs)

/*******/
static int fd = -1;
static uint8_t channel = 0;
static int error = 0;
static int isInitted = 0; 
/*******/


static int prepare(int fd) {
    
    if (ioctl(fd, SPI_IOC_WR_MODE, &MODE) == -1 ) {
        perror("Can't set MODE"); return -1; 
    }

    if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &BITS) == -1 ) {
        perror("Can't set number of BITS"); return -1; 
    }

    if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &CLOCK) == -1 ) {
        perror("Can't set WRITE CLOCK"); return -1; 
    }

    if (ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &CLOCK) == -1 ) {
        perror("Can't set READ CLOCK"); return -1; 
    }

    return 0; 
}

static uint8_t control_bits_differential(uint8_t channel) { return (channel & 7) << 4; } 
static uint8_t control_bits(uint8_t channel) { return 0x8 | control_bits_differential(channel); } 

int init(uint8_t c){
    if(isInitted){
        return 0;
    }
    fd = open(DEVICE, O_RDWR);
    if(fd <= 0){
        perror("Device open error");
        error = 1;
        return -1;
    }

    if(prepare(fd) == -1){
        perror("Device open error");
        error = 1;
        return -1;
    }
    channel = c;
    isInitted  = 1;
    error = 0;
    return 0;
}

//나중에 객체 지향으로 리팩토링하면 걍 read로 바꿀까
int readValue() {
    //초기화 되지 않았거나 error 상태면 return
    if( !isInitted || error){
        return -1;
    }

    uint8_t tx[] = {1, control_bits(channel), 0};
    uint8_t rx[3];

    struct spi_ioc_transfer tr = {
         .tx_buf = (unsigned long)tx, 
         .rx_buf = (unsigned long)rx, 
         .len = ARRAY_SIZE(tx), 
         .delay_usecs = DELAY, 
         .speed_hz = CLOCK, 
         .bits_per_word = BITS, 
    }; 
    
    if (ioctl(fd, SPI_IOC_MESSAGE(1), &tr) == 1) {
         perror("I0 Error"); 
         abort(); 
    } 
    return ((rx[1] << 8) & 0x300) | (rx[2] & 0xFF);
}

//현재 setting 다 닫고 새로 setting
int setSetting(){

}