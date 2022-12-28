#ifndef CORE_MEM_H
#define CORE_MEM_H

#include <stdint.h>
#include <stdlib.h>

// NOTE, safe malloc and realloc will exit when getting NULL.
// The try versions will not.

void *try_safe_malloc(uint8_t chnl, size_t size);
void *safe_malloc(uint8_t chnl, size_t size);

void *try_safe_realloc(void *ptr, size_t size);
void *safe_realloc(void *ptr, size_t size);

void safe_free(void *ptr);

// This will check all memory channels 
// including and above lb for memory leaks.
uint8_t check_memory_leaks(uint8_t lb);

void print_mem_chnls();

static inline uint8_t get_chnl(void *ptr) {
    return ((uint8_t *)ptr)[-1];
}

#endif
