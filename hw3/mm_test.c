#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
typedef struct memblock {
    size_t size;
    bool free;
    struct memblock* prev;
    struct memblock* next;
} memblock;


/* Function pointers to hw3 functions */
void* (*mm_malloc)(size_t);
void* (*mm_realloc)(void*, size_t);
void (*mm_free)(void*);

void load_alloc_functions() {
    void *handle = dlopen("hw3lib.so", RTLD_NOW);
    if (!handle) {
        fprintf(stderr, "%s\n", dlerror());
        exit(1);
    }

    char* error;
    mm_malloc = dlsym(handle, "mm_malloc");
    if ((error = dlerror()) != NULL)  {
        fprintf(stderr, "%s\n", dlerror());
        exit(1);
    }

    mm_realloc = dlsym(handle, "mm_realloc");
    if ((error = dlerror()) != NULL)  {
        fprintf(stderr, "%s\n", dlerror());
        exit(1);
    }

    mm_free = dlsym(handle, "mm_free");
    if ((error = dlerror()) != NULL)  {
        fprintf(stderr, "%s\n", dlerror());
        exit(1);
    }
}

int main() {
    setbuf(stdout, NULL);
    load_alloc_functions();
    
    char* bigdata = (char*) mm_malloc(sizeof(char) * 1024);
    char* bigdata2 = (char*) mm_malloc(sizeof(char) * 1024);
    memblock* blk = (void*)bigdata2 - sizeof(memblock);
    assert(!blk->free);
    
    mm_free(bigdata);
    mm_free(bigdata2);
    assert(blk->free);
    char* smalldata[4];
    smalldata[0] = mm_malloc(sizeof(char) * 300);
    assert(blk->free);
    assert(bigdata == smalldata[0]);
    smalldata[1] = mm_malloc(sizeof(char) * (512 - sizeof(memblock)));
    assert(smalldata[1] != bigdata2);
    smalldata[2] = (char*) mm_malloc(sizeof(char) * 600);
    assert(smalldata[2] == bigdata2);
    mm_free(smalldata[0]);
    mm_free(smalldata[1]);
    mm_free(smalldata[2]);
    printf("advanced malloc test successful!\n");
    return 0;
}
