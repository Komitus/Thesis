#ifndef __VECTOR__
#define __VECTOR__

#include <stdlib.h>

#define VECTOR_INIT_CAPACITY 10

typedef struct Vector
{
    size_t capacity;
    size_t size;
    void **items;
} Vector;

int vector_init(Vector *v, size_t init_size);
size_t vector_size(Vector *v);
int vector_push(Vector *v, void *item);
void vector_set(Vector *v, size_t idx, void *item);
void *vector_get(Vector *v, size_t idx);
void vector_delete(Vector *v, size_t idx);
void vector_free(Vector *v);
void vector_clear(Vector *v, void (*destructor)(void *));

#endif

