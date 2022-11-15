#ifndef UTIL_DATA_H
#define UTIL_DATA_H

#include <stdlib.h>

// Returns a positive value if both values are equal, otherwise 0.
typedef uint8_t (*equator)(void *val_ptr1, void *val_ptr2);

typedef struct util_linked_list util_ll;

util_ll *new_linked_list(uint8_t chnl, size_t cs);
void delete_linked_list(util_ll *ll);

void *ll_next(util_ll *ll);
void ll_add(util_ll *ll, void *src);

uint64_t ll_len(util_ll *ll);

uint8_t ll_contains(util_ll *ll, void *val_ptr, equator eq);

#endif
