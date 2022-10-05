#ifndef UTIL_MISC_H
#define UTIL_MISC_H

#include <stdlib.h>
#define MEM_CHANNELS 16





void *safe_malloc(uint8_t chnl, size_t size);
void safe_free(uint8_t chnl, void *ptr);
void display_channels();


#endif
