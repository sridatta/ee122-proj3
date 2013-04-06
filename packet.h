#ifndef __AW_PACKET_H__
#define __AW_PACKET_H__

#include <time.h>
#include <stdint.h>

typedef struct {
    uint32_t seq_number; /* 4 bytes */
    struct timeval timestamp; /* 8 bytes */
    uint32_t R; /* 4 bytes */
    char stream; /* 1 bytes */
    uint32_t num_expected; /* 4 bytes */
    float avg_len; /* 4 bytes */
    char garbage[128-(4+sizeof(struct timeval)+4+1+4+4)];
} ee122_packet;

#endif
