#include "ms.h"
#include "mb.h"
#include "../core_src/thread.h"
#include "../core_src/mem.h"

#include "virt.h"


typedef struct mem_space_list_entry_struct {
    struct mem_space_list_entry_struct *prev; 
    struct mem_space_list_entry_struct *next; 

    mem_block *mb;
} mem_space_list_entry;

// For sorting... we want a linked list!
struct mem_space_struct {
    addr_book * const adb;
    const uint64_t mb_min_bytes;

    // Lock for working on the list below.
    // When will this 
    pthread_rwlock_t ms_lck;

    // So, there exist memory blocks.
    // These are fixed size pieces of memory we can malloc and 
    // free into.
    //
    // The memory space struct will manage memory blocks without
    // user interference. It will allocate new memory blocks
    // when memory is needed.
    // When a user calls malloc, the memory space try and find
    // a block which can be malloced into as fast as possible.
    //
    // Can we sort memory blocks by the number of bytes they have
    // open? One single list may be kinda slow tbh..
    //
    // Could we try a single list implementation for now???
    //  
    // 
    // The list will need to be resorted after a malloc, free,
    // or shift.
    // So, just make sure these operations are atomic???
    //

    mem_space_list_entry *mb_list;
};

mem_space *new_mem_space(uint8_t chnl, uint64_t adt_cap, uint64_t mb_m_bytes) {
    mem_space *ms = safe_malloc(chnl, sizeof(mem_space));

    *(addr_book **)&(ms->adb) = new_addr_book(chnl, adt_cap);
    *(uint64_t *)&(ms->mb_min_bytes) = mb_m_bytes;

    safe_rwlock_init(&(ms->ms_lck), NULL);

    mem_space_list_entry *first_entry = 
        safe_malloc(chnl, sizeof(mem_space_list_entry));

    first_entry->prev = NULL;
    first_entry->next = NULL;

    first_entry->mb = new_mem_block(chnl, ms->adb, mb_m_bytes);

    return ms;
}

void delete_mem_space(mem_space *ms) {
    safe_wrlock(&(ms->ms_lck));

    mem_space_list_entry *iter = ms->mb_list; 
    mem_space_list_entry *next;
    while (iter) {
        next = iter->next;

        delete_mem_block(iter->mb);
        safe_free(iter);

        iter = next;
    }

    safe_rwlock_unlock(&(ms->ms_lck));

    // NOTE: must delete memory blocks before deleting 
    // Address book!
    
    delete_addr_book(ms->adb);
    safe_rwlock_destroy(&(ms->ms_lck));

    // finally, delete the memory space itself.
    safe_free(ms);
}

addr_book_vaddr ms_malloc(mem_space *ms, uint64_t min_bytes) {
    addr_book_vaddr v;
    return v;
}

void ms_free(mem_space *ms, addr_book_vaddr vaddr) {

}


