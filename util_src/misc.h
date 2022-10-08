#ifndef UTIL_MISC_H
#define UTIL_MISC_H

#include <stdint.h>
#include <stdlib.h>
#define MEM_CHANNELS 16

void *safe_malloc(uint8_t chnl, size_t size);
void safe_free(uint8_t chnl, void *ptr);
void display_channels();

#define NOTICE_MSG_BUF_LEN   50

typedef enum {
    ERROR,
    WARNING,
    POINT 
} notice_type;

typedef struct {
    notice_type type;
    uint32_t line_no;
    char message[NOTICE_MSG_BUF_LEN];
} notice;

#endif
