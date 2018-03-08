/*
 * mm_alloc.c
 *
 * Stub implementations of the mm_* routines.
 */

#include "mm_alloc.h"
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

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

void* malloc_helper(memblock* head, size_t size) {
    if (size == 0) {
        return NULL;
    }
    printf("%s block size: %i\n", head->free ? "free" : "not-free", (int)head->size);
    if (head->free && head->size >= size) {
        /* this block is free and its size is at least the requested size */
        /* this block or its subblock will be allocated */
        if (head->size / 2 > (sizeof(memblock) + size)) {
            /* split this block if size of the block is at least twice larger than requested size */
            printf("splitting block\n");
            /* construct new block */
            memblock* ptr = (void*)head + sizeof(memblock) + size;
            memset(ptr, 0, head->size - size);
            ptr->free = true;
            memblock* next = head->next;
            head->next = ptr;
            ptr->prev = head;
            ptr->next = next;
            ptr->size = head->size - size - sizeof(memblock);
            if (next) {
                next->prev = ptr;
            }

            /* shrink this block */
            head->size = size;
        } else {
            printf("reusing block\n");
            memset(get_memory_section(head), 0, size);
        }
        head->free = false;
        return get_memory_section(head);
    } else if (head->next) {
        /* if next block exists, recurse to the next block */
        return malloc_helper(head->next, size);
    } else {
        /* else, this is the end of the linked list */
        /* allocate more memory from unmapped region */
        printf("allocating new block\n");
        memblock* ptr;
        if ((ptr = sbrk(sizeof(memblock) + size)) == ((void*) -1)) {
            /* return NULL if fail to allocate more memory */
            printf("failed to allocate new block\n");
            return NULL;
        } 

        memset(ptr, 0, sizeof(memblock) + size);
        ptr->size = size;
        ptr->free = false;
        ptr->prev = head;
        head->next = ptr;
        return get_memory_section(ptr);
    }

    return NULL;
}

void *mm_malloc(size_t size) {
    /* YOUR CODE HERE */
    if (size == 0) return NULL;

    printf("<----------------\nstart allocation with size: %i\n", (int)size);
    if (head == NULL) {
        if ((head = sbrk(sizeof(memblock) + size)) == ((void*) -1)) {
            printf("failed to allocat size: %lu", size);
            return NULL;
        } else {
            memset(head, 0, sizeof(memblock) + size);
            head->free = false;
            head->size = size;
            printf("finish initialize head: %ld\n---------------->\n\n", (long)get_memory_section(head));
            return get_memory_section(head);
        }
    }

    void* ptr = malloc_helper(head, size);
    printf("finish allocation: %ld\n---------------->\n\n", (long)ptr);
    return ptr;
}

void *mm_realloc(void* ptr, size_t size) {
    mm_free(ptr);
    return mm_malloc(size);
}


void mm_free(void *ptr) {
    /* YOUR CODE HERE */
    if (ptr == NULL) {
        return;
    }

    /* assuming this is a valid ptr. the specs say nothing about error handling */
    memblock* block = ptr - sizeof(memblock);
    block->free = true;
}
