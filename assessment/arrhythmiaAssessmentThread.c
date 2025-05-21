#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>

#include "ClientInfo.h"

void* arrhythmiaAssessmentThread(void* arg) {
    CLIENT_INFO * info = (CLIENT_INFO *) arg;
    printf("%s 스레드 시작\n", info -> id);
    close(info->fd);
    free(info);
    return NULL;
}