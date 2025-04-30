# Makefile for server-side Raspberry Pi program

CC = gcc
CFLAGS = -Wall -I.
LIBS = -lcjson -lpthread

SRCS = main.c Queue.c BlockingQueue.c
OBJS = $(SRCS:.c=.o)
TARGET = serverMain

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
