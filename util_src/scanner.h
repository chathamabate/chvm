#ifndef UTIL_SCANNER_H
#define UTIL_SCANNER_H

#include "stdint.h"

// NOTE the data of a token is up to the user.
//
// If the data field of the token is non NULL,
// the object pointed to by data should reside in
// dynamic memory.
typedef struct {
    uint16_t type;
    void *data;
} token;

// Should return '\0' when exhausted.
typedef char (*char_stream)();

typedef enum {
    READY,      // More to be scanned.
    CLOSED,     // No more to be scanned. 
    ERROR       // Error occured. (Also no more to be scanned)
} scanner_state;

typedef struct _scanner {
    char_stream cs;

    scanner_state state;

    // Line number of current scan.
    uint32_t curr_line;
} scanner;

#endif
