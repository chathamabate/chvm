#ifndef CORE_MEM_H
#define CORE_MEM_H

#include <stdint.h>
#include <stdlib.h>

#define MEM_CHANNELS 16

#define MEM_CHNL_TESTING 0 

void *safe_malloc(uint8_t chnl, size_t size);
void *safe_realloc(void *ptr, size_t size);
void safe_free(uint8_t chnl, void *ptr);
void display_channels();

#endif
