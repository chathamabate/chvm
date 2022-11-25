#include "./data.h"
#include "./mem.h"
#include <stddef.h>
#include <stdlib.h>
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

static inline void slist_init_statics(slist *sl, size_t cs) {
    sl->cell_size = cs;
    sl->cap = SL_INITIAL_CAP;
    sl->len = 0;
}

slist *new_slist_unsafe(size_t cs) {
    slist *sl = malloc(sizeof(slist));
    if (!sl) {
        return sl;
    }

    slist_init_statics(sl, cs);    
    sl->buf = malloc(sl->cell_size * sl->cap);

    if (!sl->buf) {
        free(sl);
        return NULL;
    }

    return sl;
}

slist *new_slist(uint8_t chnl, size_t cs) {
    slist *sl = safe_malloc(chnl, sizeof(slist));
    slist_init_statics(sl, cs);    
    sl->buf = safe_malloc(chnl, sl->cell_size * sl->cap);

    return sl;
}

static inline void *sl_get_unsafe(slist *sl, uint64_t i) {
    return (uint8_t *)(sl->buf) + (i * sl->cell_size);
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

    return sl_get_unsafe(sl, i);
}

void sl_remove(slist *sl, uint64_t i) {
    if (i >= sl->len) {
        return;
    }
    
    // Avoid doing all this coping if not needed.
    if (i == sl->len - 1) {
        sl->len--;
        return;
    }

    uint8_t chnl = get_chnl(sl);
    size_t temp_buf_size = (sl->len - 1 - i) * sl->cell_size;
    void *temp_buf = safe_malloc(chnl, temp_buf_size);
    
    memcpy(temp_buf, sl_get(sl, i + 1), temp_buf_size);
    memcpy(sl_get(sl, i), temp_buf, temp_buf_size);

    sl->len--;

    safe_free(temp_buf);
}

