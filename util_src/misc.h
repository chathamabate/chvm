#ifndef UTIL_MISC_H
#define UTIL_MISC_H

#include <stdint.h>
#include <stdlib.h>
#define MEM_CHANNELS 16

void *safe_malloc(uint8_t chnl, size_t size);
void *safe_realloc(void *ptr, size_t size);
void safe_free(uint8_t chnl, void *ptr);
void display_channels();

typedef struct _ll_node linked_list_node;

typedef struct _ll_node {
    _ll_node *prv;
    _ll_node *nxt;

    void *data;
} linked_list_node;

typedef struct {
    uint8_t chnl;
    size_t cell_size;

    linked_list_node *head;
    linked_list_node *tail;
} linked_list;

linked_list *new_linked_list(uint8_t chnl, size_t cs);
void *ll_next(linked_list *ll);
void *delete_linked_list(linked_list *ll);

#define NOTICE_MSG_BUF_LEN   50

typedef enum {
    ERROR,
    WARNING,
    MISC
} notice_type;

typedef struct {
    notice_type type;

    // A notice could span multiple lines.
    // s and e here marks the inclusive line range.
    uint32_t line_no_s;
    uint32_t line_no_e;
    
    // Notice message.
    char message[NOTICE_MSG_BUF_LEN];
} notice;

#endif
