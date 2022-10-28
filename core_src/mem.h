#ifndef CORE_MEM_H
#define CORE_MEM_H

#include <stdint.h>
#include <stdlib.h>

#define MEM_CHANNELS 16

#define MEM_CHNL_TESTING 0 

void *safe_malloc(uint8_t chnl, size_t size);
void *safe_realloc(void *ptr, size_t size);
void safe_free(void *ptr);
void display_channels();

// This returns 1 if there is any memory used outside
// of the testing channel, 0 otherwise.
uint8_t check_memory_leaks();

// Both of the following calls return -1
// on error, 0 on success.

int safe_read(int fd, void *buf, size_t cnt);
int safe_write(int fd, void *buf, size_t cnt);

#endif
