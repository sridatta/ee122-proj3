#ifndef __AW_QUEUE_H__
#define __AW_QUEUE_H__

#include <time.h>

typedef struct {
    size_t typesize;
    unsigned capacity;
    unsigned filled;
    char* memory;
    char* head;
    char* tail;
} bytequeue;

int bytequeue_init(bytequeue* queue, size_t size, unsigned n);
int bytequeue_pop(bytequeue* queue, void* dest);
int bytequeue_push(bytequeue* queue, void* data);

#endif /* __AW_QUEUE_H__ */
