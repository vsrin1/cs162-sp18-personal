/*
 * mm_alloc.c
 *
 * Stub implementations of the mm_* routines.
 */

#include "mm_alloc.h"
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

typedef struct memblock {
    size_t size;
    bool free;
    struct memblock* prev;
    struct memblock* next;
} memblock;

memblock* head = NULL;

void* get_memory_section(memblock* ptr) {
    return (void*)ptr + sizeof(memblock);
}

memblock* get_head() {
    if (!head) {
        head = (memblock*)sbrk(0);
    }
    return head;
}

void *malloc_helper(memblock* head, size_t size) {
    if (head->free && head->size >= size) {
        /* this block is free and its size is at least the requested size */
        /* this block or its subblock will be allocated */
        if (head->size / 2 > size) {
            /* split this block if size of the block is at least twice larger than requested size */

            /* shrink this block */
            head->size = size;

            /* construct new block */
            memblock* ptr = (void*)head + sizeof(memblock) + head->size;
            ptr->free = true;
            memblock* next = head->next;
            head->next = ptr;
            ptr->prev = head;
            if (next) {
                next->prev = ptr;
                ptr->next = next;
            }
        } 
        head->free = false;
        return get_memory_section(head);
    } else {
        /* we have to find the next block otherwise */
        return NULL;

    }
}



void *mm_malloc(size_t size) {
    /* YOUR CODE HERE */
    if (size == 0) return NULL;

    return NULL;
}

void *mm_realloc(void *ptr, size_t size) {
    /* YOUR CODE HERE */
    return NULL;
}

void mm_free(void *ptr) {
    /* YOUR CODE HERE */
}
