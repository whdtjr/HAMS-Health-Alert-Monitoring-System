// Queue.h
#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>

typedef struct {
    int front, rear, current_size, max_size;
    void **array;
} Queue;

// 큐 생성
Queue* new_Queue(int max_size);

// 큐에 요소 추가
bool Queue_enq(Queue* queue, void* element);

// 큐에서 요소 제거 및 반환
void* Queue_deq(Queue* queue);

// 큐에 들어있는 요소 수 반환
int Queue_size(Queue* queue);

// 큐가 비어 있는지 확인
bool Queue_isEmpty(Queue* queue);

// 큐 비우기 (메모리는 유지)
void Queue_clear(Queue* queue);

// 큐 메모리 해제
void Queue_destroy(Queue* queue);

#endif // QUEUE_H
