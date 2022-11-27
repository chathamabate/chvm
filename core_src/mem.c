#include "mem.h"

#include <pthread.h>
#include <stdio.h>
#include "./sys.h"

void *try_safe_malloc(uint8_t chnl, size_t size) {
    uint8_t *raw_ptr = malloc(size + 1);

    if (raw_ptr) {
        raw_ptr[0] = chnl;

        _wrlock_core_state();
        _core_state->mem_chnls[chnl]++;
        _unlock_core_state();

        return (void *)(raw_ptr + 1);
    }

    return NULL;
}

void *safe_malloc(uint8_t chnl, size_t size) {
    void *ptr = try_safe_malloc(chnl, size);

    if (ptr) {
        return ptr;
    }

    core_logf(1, "Process failed to malloc.");
    safe_exit(1);

    // Should never make it here.
    return NULL;
}

void *try_safe_realloc(void *ptr, size_t size) {
    uint8_t *raw_ptr = ((uint8_t *)ptr) - 1;
    uint8_t *new_raw_ptr = realloc(raw_ptr, size + 1);

    if (new_raw_ptr) {
        return (void *)(new_raw_ptr + 1);
    }

    return NULL;
}

void *safe_realloc(void *ptr, size_t size) {
    void *new_ptr = try_safe_realloc(ptr, size);
    
    if (new_ptr) {
        return new_ptr;
    }

    core_logf(1, "Process failed to realloc.");
    safe_exit(1);

    // Should never make it here.
    return NULL;
}

void safe_free(void *ptr) {
    uint8_t *raw_ptr = ((uint8_t *)ptr) - 1;
    uint8_t chnl = raw_ptr[0];

    _wrlock_core_state();
    _core_state->mem_chnls[chnl]--;
    _unlock_core_state();

    free(raw_ptr);
}

uint8_t check_memory_leaks(uint8_t lb) {
    _rdlock_core_state();
    uint8_t chnl;
    for (chnl = lb; chnl < MEM_CHANNELS; chnl++) {
        // Check if a memory leak exists in a valid channel.
        if (_core_state->mem_chnls[chnl]) {
            _unlock_core_state();
            return 1;
        }
    }

    _unlock_core_state();
    return 0;
}

