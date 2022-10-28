#ifndef UTIL_MISC_H
#define UTIL_MISC_H

#include <stdint.h>
#include <stdlib.h>


#define NOTICE_MSG_BUF_LEN   50

typedef enum {
    ERROR,
    WARNING,
    MISC
} notice_type;

typedef struct {
    notice_type type;

    // A notice could span multiple lines.
    // s and e here marks the inclusive line range.
    uint32_t line_no_s;
    uint32_t line_no_e;
    
    // Notice message.
    char message[NOTICE_MSG_BUF_LEN];
} notice;

#endif
