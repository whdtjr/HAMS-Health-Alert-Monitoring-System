# Makefile for client-side Raspberry Pi program

CC = gcc
CFLAGS = -Wall -I. 
LIBS = -lcjson -lpthread -lm

SRCS = main.c sendFeature.c spi_reader.c
OBJS = $(SRCS:.c=.o)
TARGET = clientMain

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
