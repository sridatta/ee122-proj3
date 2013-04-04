#ifndef __AW_QUEUE_H__
#define __AW_QUEUE_H__

#include <time.h>

typedef struct {
    size_t typesize;
    unsigned capacity;
    unsigned filled;
    char* memory;
} bytequeue;

int bytequeue_init(bytequeue* queue, size_t size, unsigned n);
int bytequeue_pop(bytequeue* queue, char* dest);
int bytequeue_push(bytequeue* queue, char* data);

#endif /* __AW_QUEUE_H__ */
