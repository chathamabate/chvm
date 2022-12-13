
#include "./thread.h"

#include "../core_src/mem.h"
#include "../core_src/thread.h"

util_thread_spray_info *util_thread_spray(uint8_t chnl, uint64_t amt, 
        void *(sr)(void *), void *context) {

    util_thread_spray_context *context_wrappers = 
        safe_malloc(chnl, sizeof(util_thread_spray_context) * amt);

    pthread_t *ids = safe_malloc(chnl, sizeof(pthread_t) * amt);

    uint64_t i;
    for (i = 0; i < amt; i++) {
        context_wrappers[i].index = i;
        context_wrappers[i].context = context;

        safe_pthread_create(ids + i, NULL, sr, context_wrappers + i);
    }

    util_thread_spray_info *ts = 
        safe_malloc(chnl, sizeof(util_thread_spray_info));

    ts->amt = amt;
    ts->ids = ids;
    ts->context_wrappers = context_wrappers;

    return ts;
}

void util_thread_collect(util_thread_spray_info *spray) {
    uint64_t i;
    for (i = 0; i < spray->amt; i++) {
        safe_pthread_join(spray->ids[i], NULL);
    }

    safe_free(spray->ids);
    safe_free(spray->context_wrappers);
    safe_free(spray);
}
