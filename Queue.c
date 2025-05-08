// Queue.c
#include "Queue.h"
#include <stdlib.h>

Queue* new_Queue(int max_size) {
    if (max_size <= 0) return NULL;

    Queue *queue = malloc(sizeof(Queue));
    if (!queue) return NULL;

    queue->array = malloc(sizeof(void*) * max_size);
    if (!queue->array) {
        free(queue);
        return NULL;
    }

    queue->max_size = max_size;
    queue->front = 0;
    queue->rear = max_size - 1; // 원형 큐
    queue->current_size = 0;
    return queue;
}


// Queue *new_Queue(int max_size) {
//     if (max_size <= 0) {
//         return NULL;
//     }
//     Queue *this = malloc(sizeof(Queue));
//     this->max_size = max_size;
//     this->front = this->current_size = 0;
//     this->rear = max_size - 1;
//     this->array = malloc(this->max_size * sizeof(void*));  
//     return this;
// }


bool Queue_enq(Queue* queue, void* element) {
    if (!queue || !element || queue->current_size == queue->max_size) return false;

    queue->rear = (queue->rear + 1) % queue->max_size;
    queue->array[queue->rear] = element;
    queue->current_size++;
    return true;
}

// bool Queue_enq(Queue* this, void* element) {
//     if ((this->current_size == this->max_size) || (element == NULL)) {
//         return false;
//     } else {
//         this->rear = (this->rear + 1) % this->max_size;
//         ((void**)this->array)[this->rear] = element;
//         this->current_size++;
//         return true;
//     }
// }

void* Queue_deq(Queue* queue) {
    if (!queue || queue->current_size == 0) return NULL;

    void* item = queue->array[queue->front];
    queue->front = (queue->front + 1) % queue->max_size;
    queue->current_size--;
    return item;
}

// void* Queue_deq(Queue* this) {
//     if (Queue_isEmpty(this)) {
//         return NULL;
//     }
//     void* item = ((void**)this->array)[this->front];
//     this->front = (this->front + 1) % this->max_size;
//     this->current_size--;
//     return item;
// }

int Queue_size(Queue* queue) {
    return queue ? queue->current_size : 0;
}

bool Queue_isEmpty(Queue* queue) {
    return queue ? queue->current_size == 0 : true;
}

void Queue_clear(Queue* queue) {
    if (!queue) return;
    queue->front = 0;
    queue->rear = queue->max_size - 1;
    queue->current_size = 0;
}
// void Queue_clear(Queue* this) {
//     this->current_size = 0;
//     this->front = 0;
//     this->rear = this->max_size - 1;
// }
void Queue_destroy(Queue* queue) {
    if (!queue) return;
    free(queue->array);
    free(queue);
}

// void Queue_destroy(Queue* this) {
//     free(this->array);
//     free(this);
// }

void Queue_print(Queue* queue, void (*print_func)(void*)) {
    if (!queue || queue->current_size == 0) {
        printf("Queue is empty.\n");
        return;
    }

    int temp = queue->front;
    for (int i = 0; i < queue->current_size; i++) {
        printf("[%d]th element:  ", i);
        print_func(queue->array[temp]);
        printf("\n");
        temp = (temp + 1) % queue->max_size;
    }
}
