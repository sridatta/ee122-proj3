#include "queue.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int bytequeue_init(bytequeue* queue, size_t size, unsigned n) {
    queue->memory = malloc(size * n);
    queue->typesize = size;
    queue->capacity = n;
    queue->filled = 0;
    if (queue->memory == NULL) {
        return -1;
    }
    return 0;
}

int bytequeue_pop(bytequeue* queue, void* dest) {
    if (queue->filled == 0) {
        return -1;
    }
    queue->memory -= queue->typesize;
    queue->filled--;
    memcpy(dest, queue->memory, queue->typesize);
    return 0;
}

int bytequeue_push(bytequeue* queue, void* data) {
    if (queue->filled == queue->capacity) {
        return -1;
    }
    memcpy(queue->memory, data, queue->typesize);
    queue->filled++;
    queue->memory += queue->typesize;
    return 0;
}
