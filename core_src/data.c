#include "./data.h"
#include "./mem.h"
#include <stddef.h>
#include <string.h>

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

void sl_add(slist *sl, void *buf) {
    if (sl->len == sl->cap) {
        // Realloc time.
        sl->cap *= 2;
        sl->buf = safe_realloc(sl->buf, sl->cap * sl->cell_size);
    }

    // Get pointer of next cell.
    void *dest = ((uint8_t *)sl->buf) + (sl->len * sl->cell_size);
    memcpy(dest, buf, sl->cell_size);
    sl->len++;
}

