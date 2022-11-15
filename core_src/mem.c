#include "mem.h"

#include <stdio.h>

static uint32_t mem_chnls[MEM_CHANNELS];

void *safe_malloc(uint8_t chnl, size_t size) {
    uint8_t *raw_ptr = malloc(size + 1);

    if (raw_ptr) {
        raw_ptr[0] = chnl;
        mem_chnls[chnl]++;

        return (void *)(raw_ptr + 1);
    }

    printf("[Malloc] Process out of memory!\n");
    display_channels();

    exit(1);
}

void *safe_realloc(void *ptr, size_t size) {
    uint8_t *raw_ptr = ((uint8_t *)ptr) - 1;
    uint8_t *new_raw_ptr = realloc(raw_ptr, size + 1);

    if (new_raw_ptr) {
        return (void *)(new_raw_ptr + 1);
    }

    printf("[Realloc] Process out of memory!\n");
    display_channels();

    exit(1);
}

void safe_free(void *ptr) {
    uint8_t *raw_ptr = ((uint8_t *)ptr) - 1;
    uint8_t chnl = raw_ptr[0];
    mem_chnls[chnl]--;

    free(raw_ptr);
}

#define DISPLAY_CHNL_ROW_WIDTH 4

void display_channels() {
    uint8_t i; 

    for (i = 0; i < MEM_CHANNELS; i++) {
        printf("%d: %d", i, mem_chnls[i]);

        if ((i + 1) % DISPLAY_CHNL_ROW_WIDTH == 0) {
            printf("\n");
        } else {
            printf(", ");
        }
    }
}

uint8_t check_memory_leaks() {
    uint8_t chnl;
    for (chnl = 1; chnl < MEM_CHANNELS; chnl++) {
        if (mem_chnls[chnl]) {
            return 1;
        }
    }

    return 0;
}

uint8_t get_chnl(void *ptr) {
    return ((uint8_t *)ptr)[-1];
}

