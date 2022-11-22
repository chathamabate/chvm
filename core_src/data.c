#include "./data.h"
#include "./mem.h"
#include <stddef.h>
#include <string.h>

char *concat_str(uint8_t chnl, const char *s1, const char *s2) {
    unsigned long l1 = strlen(s1);
    unsigned long l2 = strlen(s2);

    char *new_str = safe_malloc(chnl, 
            (l1 + l2 + 1) * sizeof(char));

    memcpy(new_str, s1, l1);
    memcpy(new_str + l1, s2, l2);
    new_str[l1 + l2] = '\0';

    return new_str;
}

#define SL_INITIAL_CAP 1

slist *new_slist(uint8_t chnl, size_t cs) {
    slist *sl = safe_malloc(chnl, sizeof(slist));
    
    sl->cell_size = cs;
    sl->cap = SL_INITIAL_CAP;
    sl->len = 0;

    sl->buf = safe_malloc(chnl, sl->cell_size * sl->cap);

    return sl;
}

void sl_add(slist *sl, void *buf) {
    if (sl->len == sl->cap) {
        // Realloc time.
        sl->cap *= 2;
        sl->buf = safe_realloc(sl->buf, sl->cap * sl->cell_size);
    }

    // Get pointer of next cell.
    void *dest = (uint8_t *)(sl->buf) + (sl->len * sl->cell_size);
    memcpy(dest, buf, sl->cell_size);
    sl->len++;
}

void *sl_get(slist *sl, uint64_t i) {
    if (i >= sl->len) {
        return NULL;
    }

    return (uint8_t *)(sl->buf) + (i * sl->cell_size);
}

