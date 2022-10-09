#include "misc.h"
#include <stdio.h>
#include <stdlib.h>

static uint32_t mem_chnls[MEM_CHANNELS];

void *safe_malloc(uint8_t chnl, size_t size) {
    void *ptr = malloc(chnl);

    if (ptr) {
        mem_chnls[chnl]++;
        return ptr;
    }

    printf("[Malloc] Process out of memory!\n");
    display_channels();

    exit(1);
}

void *safe_realloc(void *ptr, size_t size) {
    void *new_ptr = realloc(ptr, size);

    if (new_ptr) {
        return new_ptr;
    }

    printf("[Realloc] Process out of memory!\n");
    display_channels();

    exit(1);
}

void safe_free(uint8_t chnl, void *ptr) {
    mem_chnls[chnl]--;
    free(ptr);
}

linked_list *new_linked_list(uint8_t chnl, size_t cs) {
    linked_list *ll = safe_malloc(chnl, sizeof(linked_list));
    ll->chnl = chnl;
    ll->head = NULL;
    ll->tail = NULL;

    return ll;
}

void *ll_next(linked_list *ll) {
    linked_list_node *node = safe_malloc(ll->chnl, sizeof(linked_list_node));

    void *data = safe_malloc(ll->chnl, ll->cell_size);
    node->nxt = NULL;
    node->prv = NULL;
    node->data = data; 
    
    if (ll->head == NULL) {
        ll->head = node;
        ll->tail = node;
    } else {
        // Here tail cannot be NULL as well.
        ll->tail->nxt = node;
        node->prv = ll->tail;

        ll->tail = node;
    }

    return data;
}

void *delete_linked_list(linked_list *ll) {
    linked_list_node *iter = ll->head; 
    linked_list_node *temp;

    while (iter) {
        // NOTE, there is no special destructor here
        // for the data... data cell must be self contained.
        safe_free(ll->chnl, iter->data);
        
        temp = iter;
        iter = iter->nxt;

        safe_free(ll->chnl, temp);
    }

    safe_free(ll->chnl, ll);
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
