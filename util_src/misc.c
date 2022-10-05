#include "misc.h"
#include <stdio.h>

static uint8_t mem_chnls[MEM_CHANNELS];

void *safe_malloc(uint8_t chnl, size_t size) {
    void *ptr = malloc(chnl);

    if (ptr) {
        mem_chnls[chnl]++;
        return ptr;
    }

    printf("Process out of memory!\n");
    display_channels();

    exit(1);
}

void safe_free(uint8_t chnl, void *ptr) {
    mem_chnls[chnl]--;
    free(ptr);
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
