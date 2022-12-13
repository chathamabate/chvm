#ifndef UTIL_THREAD_H
#define UTIL_THREAD_H

#include <stdint.h>
#include <pthread.h>

typedef struct {
    uint64_t index; 
    void *context;
} util_thread_spray_context;

typedef struct {
    uint64_t amt;
    pthread_t *ids;

    // These wrappers are what will be sent to each 
    // individual thread spawned by the spray.
    util_thread_spray_context *context_wrappers;
} util_thread_spray_info;

// This is a helper function for spawning many threads at the exact
// same time. 
// Each thread will run the same routine and receive the same context argument
// wrapped in a util_thread_spray_context object.
util_thread_spray_info *util_thread_spray(uint8_t chnl, uint64_t amt, 
        void *(sr)(void *), void *context);

// This will wait for all threads in a spray to return.
// NOTE: spray will be deleted after use here!
void util_thread_collect(util_thread_spray_info *spray);

#endif
