#ifndef CLIENTINFO_H
#define CLIENTINFO_H

#define ID_SIZE 10

typedef struct {
    int fd;
    char ip[20];
    char id[ID_SIZE];

}CLIENT_INFO;
#endif // CLIENTINFO_H