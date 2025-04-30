#ifndef SPI_READER_H
#define SPI_READER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int init(uint8_t channel);  
int readValue(void);             

#ifdef __cplusplus
}
#endif

#endif // SPI_READER_H
