#ifndef CORE_MEM_H
#define CORE_MEM_H

#include <stdint.h>
#include <stdlib.h>

#define MEM_CHANNELS 16

#define MEM_CHNL_TESTING 0 


// NOTE, safe malloc and realloc will exit when getting NULL.
// The try versions will not.

void *try_safe_malloc(uint8_t chnl, size_t size);
void *safe_malloc(uint8_t chnl, size_t size);

void *try_safe_realloc(void *ptr, size_t size);
void *safe_realloc(void *ptr, size_t size);

void safe_free(void *ptr);
void display_channels();

// This returns 1 if there is any memory used outside
// of the testing channel, 0 otherwise.
uint8_t check_memory_leaks();

static inline uint8_t get_chnl(void *ptr) {
    return ((uint8_t *)ptr)[-1];
}

#endif
