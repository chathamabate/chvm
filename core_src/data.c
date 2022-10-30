#include "./data.h"
#include "./mem.h"
#include <stddef.h>

#define SL_INITIAL_CAP 1

slist *new_slist(uint8_t chnl, size_t cs) {
    slist *sl = safe_malloc(chnl, sizeof(slist));
    
    sl->cell_size = cs;
    sl->cap = SL_INITIAL_CAP;
    sl->len = 0;

    sl->buf = safe_malloc(chnl, sl->cell_size * sl->cap);

    return sl;
}

void delete_slist(slist *sl) {
    safe_free(sl->buf);
    safe_free(sl);
}

void *sl_next(slist *sl) {
    if (sl->len == sl->cap) {
        // Realloc time.
        sl->cap *= 2;
        sl->buf = safe_realloc(sl->buf, sl->cap * sl->cell_size);
    }

    // Get pointer of next cell.
    void *ptr = ((uint8_t *)sl->buf) + (sl->len * sl->cell_size);

    // Push length.
    sl->len++;

    return ptr;
}
