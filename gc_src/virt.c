#include "./virt.h"
#include <pthread.h>
#include "../core_src/sys.h"
#include "../core_src/thread.h"
#include "../core_src/mem.h"

// The structure of the address table in memory is going
// to be a bit funky.

typedef struct {
    // This needs no lock as it will alwyas be
    // constant.
    const uint64_t cap;

    // Mutex for cyclic free queue.
    // This must be locked to modify any
    // of the below variables.
    pthread_mutex_t free_q_mut;

    // How many items are in the queue.
    // If this is 0, the table is full.
    uint64_t q_fill;

    // Inclusive range.
    uint64_t q_s, q_e;
} addr_table_header;

// After the header there will be cap * uint64_t indeces.
// These will form a cyclic queue representing every free
// entry in the adt so far.

// after this will be the cells themselves.

typedef struct {
    pthread_rwlock_t lck; 
    void *paddr;
} addr_table_cell;

addr_table *new_addr_table(uint8_t chnl, uint64_t cap) {
    addr_table *adt = safe_malloc(chnl, 
            sizeof(addr_table_header) + 
            (sizeof(uint64_t) * cap) + 
            (sizeof(addr_table_cell) * cap)
    );

    addr_table_header *adt_h = (addr_table_header *)adt;
    uint64_t *free_q = (uint64_t *)(adt_h + 1);
    addr_table_cell *table = (addr_table_cell *)(free_q + cap);

    // Init the header.
    //
    // Hacky way of setting a constant.
    *(uint64_t *)&(adt_h->cap) = cap;
    safe_mutex_init(&(adt_h->free_q_mut), NULL);
    adt_h->q_fill = 0;
    adt_h->q_s = 0;
    adt_h->q_e  = 0;

    // Init the free queue.
    // This will hold all cells at first.
    
    uint64_t i;
    for (i = 0; i < cap; i++) {
        free_q[i] = i;
    }

    // Init each cell now.
    for (i = 0; i < cap; i++) {
        safe_rwlock_init(&(table[i].lck), NULL);
        table[i].paddr = NULL;  // Not necessary, but whatevs.
    }

    return adt;
}

void delete_addr_table(addr_table *adt) {
    addr_table_header *adt_h = (addr_table_header *)adt;
    uint64_t *free_q = (uint64_t *)(adt_h + 1);
    addr_table_cell *table = (addr_table_cell *)(free_q + adt_h->cap);

    safe_mutex_destroy(&(adt_h->free_q_mut));

    uint64_t i;
    for (i = 0; i < adt_h->cap; i++) {
        safe_rwlock_destroy(&(table[i].lck));
    }

    safe_free(adt);
}

addr_table_put_res adt_put(addr_table *adt, void *paddr) {
    addr_table_header *adt_h = (addr_table_header *)adt;
    uint64_t *free_q = (uint64_t *)(adt_h + 1);
    addr_table_cell *table = (addr_table_cell *)(free_q + adt_h->cap);

    addr_table_put_res res;

    uint64_t free_ind;
    safe_mutex_lock(&(adt_h->free_q_mut));

    if (adt_h->q_fill == 0) {
        res.code = ADT_NO_SPACE;
        safe_mutex_unlock(&(adt_h->free_q_mut));

        return res;
    }

    // If we make it here threre will always be a free cell
    // in the table. (Just need to pop it off the queuee)

    // NOTE: are queue most pop off from the front...
    // push to the back.
    free_ind = free_q[adt_h->q_s];
    adt_h->q_fill--;

    // Only push queue start forward if there are more
    // elements in the queue.
    if (adt_h->q_fill > 0) {
        adt_h->q_s = (adt_h->q_s + 1) % adt_h->cap;
        res.code = ADT_SUCCESS;
    } else {
        // Here is the case where we have popped the
        // last cell index off the queue.
        // Our table is now full.
        res.code = ADT_NEWLY_FULL;
    }

    safe_mutex_unlock(&(adt_h->free_q_mut));

    // We have aquired our free index...
    // Now to write to it.
    safe_wrlock(&(table[free_ind].lck));
    table[free_ind].paddr = paddr;
    safe_rwlock_unlock(&(table[free_ind].lck));

    res.index = free_ind;

    return res;
}

// Edit the paddr of a cell in the table.
void adt_set(addr_table *adt, uint64_t ind, void *paddr) {
    addr_table_header *adt_h = (addr_table_header *)adt;
    uint64_t *free_q = (uint64_t *)(adt_h + 1);
    addr_table_cell *table = (addr_table_cell *)(free_q + adt_h->cap);

    addr_table_cell *cell = table + ind;

    safe_wrlock(&(cell->lck));
    cell->paddr = paddr;
    safe_rwlock_unlock(&(cell->lck));
}

// Get the physical address at ind.
// The read lock will be requested on the address.
void *adt_get_read(addr_table *adt, uint64_t ind) {
    addr_table_header *adt_h = (addr_table_header *)adt;
    uint64_t *free_q = (uint64_t *)(adt_h + 1);
    addr_table_cell *table = (addr_table_cell *)(free_q + adt_h->cap);

    addr_table_cell *cell = table + ind;

    safe_rdlock(&(cell->lck));

    return cell->paddr;
}

// Same as adt_get_read, except with a write lock.
void *adt_get_write(addr_table *adt, uint64_t ind) {
    addr_table_header *adt_h = (addr_table_header *)adt;
    uint64_t *free_q = (uint64_t *)(adt_h + 1);
    addr_table_cell *table = (addr_table_cell *)(free_q + adt_h->cap);
    
    addr_table_cell *cell = table + ind;

    safe_wrlock(&(cell->lck));

    return cell->paddr;
}

// Unlock the entry at index.  
void adt_unlock(addr_table *adt, uint64_t ind) {
    addr_table_header *adt_h = (addr_table_header *)adt;
    uint64_t *free_q = (uint64_t *)(adt_h + 1);
    addr_table_cell *table = (addr_table_cell *)(free_q + adt_h->cap);

    addr_table_cell *cell = table + ind;

    safe_rwlock_unlock(&(cell->lck));
}

addr_table_code adt_free(addr_table *adt, uint64_t index) {
    addr_table_header *adt_h = (addr_table_header *)adt;
    uint64_t *free_q = (uint64_t *)(adt_h + 1);

    addr_table_code res_code;

    safe_mutex_lock(&(adt_h->free_q_mut));

    // Case where there are no elements in the queue at
    // the start of the free.
    if (adt_h->q_fill == 0) {
        res_code = ADT_NEWLY_FREE;
    } else {
        res_code = ADT_SUCCESS;
    }

    uint64_t new_tail = (adt_h->q_e + 1) % adt_h->cap;
    free_q[new_tail] = index;

    adt_h->q_fill++;

    safe_mutex_unlock(&(adt_h->free_q_mut));

    return res_code;
}
