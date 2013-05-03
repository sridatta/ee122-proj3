#include "queue.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int bytequeue_init(bytequeue* queue, size_t size, unsigned n) {
    queue->memory = malloc(size * n);
    queue->end = queue->memory + (size * n);
    queue->typesize = size;
    queue->capacity = n;
    queue->filled = 0;
    queue->tail = queue->memory;
    queue->head = queue->memory;
    if (queue->memory == NULL) {
        return -1;
    }
    return 0;
}

int bytequeue_pop(bytequeue* queue, void* dest) {
    if (queue->filled == 0) {
        return -1;
    }
    memcpy(dest, queue->head, queue->typesize);
    queue->filled--;
    queue->head += queue->typesize;
    if (queue->head == queue->end)
      queue->head = queue->memory;
    return 0;
}

int bytequeue_push(bytequeue* queue, void* data) {
    if (queue->filled == queue->capacity) {
        return -1;
    }
    memcpy(queue->tail, data, queue->typesize);
    queue->filled++;
    queue->tail += queue->typesize;
    if (queue->tail == queue->end)
      queue->tail = queue->memory;
    return 0;
}
