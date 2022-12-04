#include "./data.h"
#include "../core_src/mem.h"
#include <string.h>
#include <stdio.h>

typedef struct util_linked_list_node_header util_ll_node_header;

typedef struct util_linked_list_node_header {
    util_ll_node_header *nxt;
    util_ll_node_header *prv;
} util_ll_node_header;

typedef struct util_linked_list {
    size_t cell_size;

    uint64_t len;

    util_ll_node_header *start;
    util_ll_node_header *end;
} util_ll;

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



