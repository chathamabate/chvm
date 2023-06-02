#include "./data.h"
#include "../core_src/mem.h"
#include "../core_src/sys.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>

typedef struct util_linked_list_node_header util_ll_node_header;

typedef struct util_linked_list_node_header {
    util_ll_node_header *nxt;
    util_ll_node_header *prv;
} util_ll_node_header;

struct util_linked_list {
    size_t cell_size;

    uint64_t len;

    util_ll_node_header *start;
    util_ll_node_header *end;
}; // util_ll;

util_ll *new_linked_list(uint8_t chnl, size_t cs) {
    if (cs == 0) {
        return NULL;
    }

    util_ll *ll = safe_malloc(chnl, sizeof(util_ll));

    ll->cell_size = cs;
    ll->len = 0;
    ll->start = NULL;
    ll->end = NULL;

    return ll;
}

void delete_linked_list(util_ll *ll) {
    util_ll_node_header *iter = ll->start;
    util_ll_node_header *temp_iter;

    while (iter) {
        temp_iter = iter->nxt;

        // Free node itself.
        safe_free(iter);

        iter = temp_iter;
    }

    safe_free(ll);
}

void *ll_next(util_ll *ll) {
    uint8_t chnl = get_chnl(ll);

    util_ll_node_header *next = 
        safe_malloc(chnl, sizeof(util_ll_node_header) + ll->cell_size);
    
    // Start of the data section of the node.
    void *data = &(next[1]);
    
    next->prv = ll->end;
    next->nxt = NULL;
    
    if (ll->start == NULL) {
        ll->start = next;
        ll->end = next;
    } else {
        ll->end->nxt = next;
    }
    
    ll->len++;

    return data;
}

void ll_add(util_ll *ll, void *src) {
    memcpy(ll_next(ll), src, ll->cell_size);
}

void *ll_get(util_ll *ll, uint64_t i) {
    if (i >= ll->len) {
        return NULL;
    }

    uint64_t c;
    util_ll_node_header *iter = ll->start;
    for (c = 0; c < i; c++) {
        iter = iter->nxt;
    } 

    return &(iter[1]);
}

uint64_t ll_len(util_ll *ll) {
    return ll->len;
}

struct util_array_list {
    size_t cell_size;
    uint64_t cap;
    void *array; // size(array) = cell_size * cap.
                 
    uint64_t len;
};

util_ar *new_array_list(uint8_t chnl, size_t cs) {
    if (cs == 0) {
        return NULL;
    }

    util_ar *ar = safe_malloc(chnl, sizeof(util_ar));

    ar->cell_size = cs;

    ar->cap = 1;
    ar->array = safe_malloc(chnl, ar->cell_size * ar->cap);

    ar->len = 0;

    return ar;
}

void delete_array_list(util_ar *ar) {
    safe_free(ar->array);

    safe_free(ar);
}

void *ar_next(util_ar *ar) {
    if (ar->len == ar->cap) {
        uint64_t new_cap = (ar->cap * 2) + 1;

        ar->array = safe_realloc(ar->array, new_cap);
        ar->cap = new_cap;
    }

    return (uint8_t *)(ar->array) + (ar->len++ * ar->cell_size);
}

void ar_add(util_ar *ar, void *src) {
    memcpy(ar_next(ar), src, ar->cell_size);
}

void *ar_get(util_ar *ar, uint64_t i) {
    return (uint8_t *)(ar->array) + (i * ar->cell_size);
}

uint64_t ar_len(util_ar *ar) {
    return ar->len;
}

typedef struct util_bc_table_header_struct {
    struct util_bc_table_header_struct *next;
    struct util_bc_table_header_struct *prev;
} util_bc_table_header;

struct util_broken_collection {
    size_t cell_size;
    uint64_t table_size;

    uint8_t delete_empty_tables;

    // NOTE: the table chain will always hold at least
    // 1 table.
    //
    // The table chain will never shrink.
    // That is even when tables in the chain become empty,
    // they will remain in the chain to prevent further mallocs.
    //
    // first will be the pointer to the first table which contains an
    // element. Or an arbitrary table if the bc is empty.
    // 
    // last will be the pointer to the last table which contains an
    // element, Or it will equal first if the bc is empty.
    //
    // if first = last & first_start = last_end, the bc is empty.
    //
    // last_end will always be a valid index to place into.
    // That is last_end is always < table_size.

    util_bc_table_header *first;

    // Index in first of the first element.
    uint64_t first_start; 

    util_bc_table_header *last;

    // Index in last of the first free space. (i.e. Exclusive)
    uint64_t last_end;
};

static inline util_bc_table_header *new_util_bc_table(util_bc *bc) {
    util_bc_table_header *bc_t_h = safe_malloc(get_chnl(bc), 
            sizeof(util_bc_table_header) +
            (bc->cell_size * bc->table_size));

    bc_t_h->next = NULL;
    bc_t_h->prev = NULL;

    return bc_t_h;
}

util_bc *new_broken_collection(uint8_t chnl, size_t cs, uint64_t ts,
        uint8_t del_empty_tables) {
    if (cs == 0 || ts == 0) {
        error_logf(1, 1, "new_broken_collection: bad sizes given");
    }

    util_bc *bc = safe_malloc(chnl, sizeof(util_bc));

    bc->cell_size = cs;
    bc->table_size = ts;
    bc->delete_empty_tables = del_empty_tables;

    util_bc_table_header *first_table = new_util_bc_table(bc);

    bc->first = first_table;
    bc->first_start = 0;

    bc->last = first_table;
    bc->last_end = 0;

    return bc;
}

void delete_broken_collection(util_bc *bc) {
    util_bc_table_header *iter, *next;

    iter = bc->first->prev;
    while (iter) {
        next = iter->prev;
        safe_free(iter);
        iter = next;
    }

    iter = bc->first;
    while (iter) {
        next = iter->next;
        safe_free(iter);
        iter = next;
    }

    safe_free(bc);
}

uint64_t bc_get_num_tables(util_bc *bc) {
    uint64_t num_tables = 0;

    util_bc_table_header *iter = bc->first->prev;

    while (iter) {
        num_tables++;
        iter = iter->prev;
    }

    iter = bc->first;

    while (iter) {
        num_tables++;
        iter = iter->next;
    }
    
    return num_tables;
}

static inline uint8_t bc_empty_inline(util_bc *bc) {
    return bc->first == bc->last && bc->first_start == bc->last_end;
}

uint8_t bc_empty(util_bc *bc) {
    return bc_empty_inline(bc);
}

void bc_push_back(util_bc *bc, const void *src) {
    // We know last_end will always be valid to place into.
    
    uint8_t *table = (uint8_t *)(bc->last + 1);
    memcpy(table + (bc->last_end * bc->cell_size), src, bc->cell_size);

    bc->last_end++;

    // Here, our last_end is still valid, no work needs to be done.
    if (bc->last_end < bc->table_size) {
        return;
    }

    // Here there is no next table in our chain!!!
    // We must malloc.
    if (!(bc->last->next)) {
        util_bc_table_header *new_table = new_util_bc_table(bc);

        bc->last->next = new_table;
        new_table->prev = bc->last;
    }

    // Advance to the next table.
    bc->last = bc->last->next;
    bc->last_end = 0;
}

void bc_push_front(util_bc *bc, const void *src) {
    // Here we must back up in the chain.
    if (bc->first_start == 0) {
        // If there is no where to back up to, malloc.
        if (!(bc->first->prev)) {
            util_bc_table_header *new_table = new_util_bc_table(bc);

            bc->first->prev = new_table;
            new_table->next = bc->first;
        }

        bc->first = bc->first->prev;
        bc->first_start = bc->table_size;
    }

    bc->first_start--;

    uint8_t *table = (uint8_t *)(bc->first + 1);
    memcpy(table + (bc->first_start * bc->cell_size), src, bc->cell_size);
}

void bc_pop_back(util_bc *bc, void *dest) {
    if (bc_empty_inline(bc)) {
        error_logf(1, 1, "bc_pop_back: cannot pop from empty bc");
    }

    // Here bc is not empty, so there must be somewhere to back up to.
    if (bc->last_end == 0) {
        bc->last = bc->last->prev;
        bc->last_end = bc->table_size;

        if (bc->delete_empty_tables) {
            safe_free(bc->last->next);
            bc->last->next = NULL;
        }
    }     

    bc->last_end--;

    uint8_t *table = (uint8_t *)(bc->last + 1);
    memcpy(dest, table + (bc->last_end * bc->cell_size), bc->cell_size);
}

void bc_pop_front(util_bc *bc, void *dest) {
    if (bc_empty_inline(bc)) {
        error_logf(1, 1, "bc_pop_back: cannot pop from empty bc");
    }

    uint8_t *table = (uint8_t *)(bc->first + 1);
    memcpy(dest, table + (bc->first_start * bc->cell_size), bc->cell_size);

    bc->first_start++;

    // Remember, because our boy wasn't empty...
    // in the case below, there is no need to check for a next table
    // we know it must exists.

    if (bc->first_start == bc->table_size) {
        bc->first_start = 0;
        bc->first = bc->first->next;

        if (bc->delete_empty_tables) {
            safe_free(bc->first->prev); 
            bc->first->prev = NULL;
        }
    }
}

void bc_foreach(util_bc *bc, cell_consumer c, void *ctx) {
    util_bc_table_header *iter = bc->first;
    uint64_t cell_ind = bc->first_start;

    uint8_t *table = (uint8_t *)(iter + 1);
    
    while (!(iter == bc->last && cell_ind == bc->last_end)) {
        void *cell = table + (cell_ind * bc->cell_size);
        c(cell, ctx);

        // Advance.
        cell_ind++;

        // Advance to next block if needed.
        if (cell_ind == bc->table_size) {
            cell_ind = 0;
            iter = iter->next;
            table = (uint8_t *)(iter + 1);
        }
    }
}

static void bc_len_consumer(void *cell, void *ctx) {
    (*(uint64_t *)ctx)++;
}

// NOTE : THIS CAN BE GREATLY IMPROVED.
uint64_t bc_len(util_bc *bc) {
    uint64_t len = 0; 
    bc_foreach(bc, bc_len_consumer, &len);

    return len;
}

