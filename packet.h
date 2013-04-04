#ifndef __AW_PACKET_H__
#define __AW_PACKET_H__

#include <time.h>


typedef struct {
    unsigned long seq_number; /* 4 bytes */
    struct timeval timestamp; /* 8 bytes */
    unsigned long R; /* 4 bytes */
    char stream; /* 1 bytes */
    char garbage[128-(4+8+4+1)];
} packet;

#endif
