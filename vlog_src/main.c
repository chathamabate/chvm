#include "./main.h"
#include "../core_src/io.h"

#include "../gc_src/virt.h"
#include "../gc_src/mb.h"
#include "../gc_src/cs.h"
#include <stdint.h>
#include <time.h>

#define VLOG_CHANNEL 6

static const struct timespec T = {
    .tv_nsec = 100000000,
};

static const gc_worker_spec GC_S = {
    .delay = &T,
    .shift = 1,
    .shift_trigger = 5,
};

int vlog_main() {
    collected_space *cs = new_collected_space(VLOG_CHANNEL, 5, 1000);

    cs_start_gc(cs, &GC_S);

    cs_root_id rid = cs_malloc_root(cs, 1, 0);
    addr_book_vaddr r = cs_get_root_vaddr(cs, rid).root_vaddr;


    obj_index root_ind = cs_get_write_ind(cs, r);
    root_ind.rt[0] = cs_malloc_object(cs, 0, sizeof(int64_t));
    cs_unlock(cs, r);

    // Did some work here....

    root_ind = cs_get_write_ind(cs, r);
    root_ind.rt[0] = NULL_VADDR;
    cs_unlock(cs, r);

    uint64_t i;
    for (i = 0; i < 1000000000; i++);

    cs_stop_gc(cs);
    
    cs_print(cs);

    delete_collected_space(cs);


    return 0;
}

