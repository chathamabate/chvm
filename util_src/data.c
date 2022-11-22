#include "./data.h"
#include "../core_src/mem.h"
#include <string.h>
#include <stdio.h>

typedef struct util_linked_list_node util_ll_node;

typedef struct util_linked_list_node {
    util_ll_node *nxt;
    util_ll_node *prv;

    void *data;
} util_ll_node;

typedef struct util_linked_list {
    size_t cell_size;

    uint64_t len;

    util_ll_node *start;
    util_ll_node *end;
} util_ll;

// Should containers store pointers???
// Should the store pointers to data cells???
// With data cells, we can have variable sized items
// that can be deleted entirely by the list itself!
// I personally like this more!
// Fuck this init stuff, simply add a pointer if thats easier!

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
    util_ll_node *iter = ll->start;
    util_ll_node *temp_iter;

    while (iter) {
        temp_iter = iter->nxt;
        
        // Free data cell.
        safe_free(iter->data);

        // Free node itself.
        safe_free(iter);

        iter = temp_iter;
    }

    safe_free(ll);
}

void *ll_next(util_ll *ll) {
    uint8_t chnl = get_chnl(ll);

    util_ll_node *next = safe_malloc(chnl, sizeof(util_ll_node));
    next->data = safe_malloc(chnl, ll->cell_size);
    
    next->prv = ll->end;
    next->nxt = NULL;
    
    if (ll->start == NULL) {
        ll->start = next;
        ll->end = next;
    } else {
        ll->end->nxt = next;
    }
    
    ll->len++;

    return next->data;
}

void ll_add(util_ll *ll, void *src) {
    memcpy(ll_next(ll), src, ll->cell_size);
}

uint64_t ll_len(util_ll *ll) {
    return ll->len;
}

uint8_t ll_contains(util_ll *ll, void *val_ptr, equator eq) {
    util_ll_node *iter = ll->start;

    while (iter) {
        if (eq(iter->data, val_ptr)) {
            return 1;
        }

        iter = iter->nxt;
    }

    return 0;
}


