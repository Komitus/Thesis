#include "vector.h"
#include <stdio.h>
#include <errno.h>
#include "structs.h"

int vector_init(Vector *v, size_t init_size)
{
    if (init_size > __INT32_MAX__)
    {
        init_size = __INT32_MAX__ / 4;
    }
    v->capacity = init_size > 0 ? init_size : VECTOR_INIT_CAPACITY;
    v->size = 0;
    v->items = malloc(sizeof(void *) * v->capacity);
    if (v->items == NULL)
    {
        fprintf(stderr, "Out of memory\n");
        return FAIL_STATUS;
    }

    return SUCCES_STATUS;
}

size_t vector_size(Vector *v)
{
    return v->size;
}

static int vector_resize(Vector *v, size_t newSize)
{
    void **items = realloc(v->items, newSize * sizeof(void *));

    if (items == NULL)
    {
        return FAIL_STATUS;
    }
    else
    {
        v->items = items;
        v->capacity = newSize;
    }
    DEBUG_LOG("VEC RESIZE %lu\n", v->capacity);
    return SUCCES_STATUS;
}

int vector_push(Vector *v, void *item)
{
    if (v->capacity == v->size){
        if(vector_resize(v, v->capacity * 2) == FAIL_STATUS){
            return FAIL_STATUS;
        }
    }
    v->items[v->size++] = item;
    return SUCCES_STATUS;
}

void vector_set(Vector *v, size_t idx, void *item)
{
    if (idx < v->size)
        v->items[idx] = item;
}
void *vector_get(Vector *v, size_t idx)
{
    if (idx < v->size)
        return v->items[idx];
    return NULL;
}
/**
 * @brief delete item pointer from array from given idx memory 
 * item v->items[idx] needs to be free manually!!!
 * @param v
 * @param idx
 */
void vector_delete(Vector *v, size_t idx)
{
    if (idx >= v->size)
        return;

    v->items[idx] = NULL;

    for (size_t i = idx; i < v->size - 1; i++)
    {
        v->items[i] = v->items[i + 1];
    }
    v->items[v->size - 1] = NULL;
    v->size--;

    if (v->size > 0 && v->size == v->capacity / 4)
        vector_resize(v, v->capacity / 2);
}

/**
 * @brief free array of pointers of given vector
 * items that pointers point to need to be free manualy!!!
 * @param v 
 */
void vector_free(Vector *v)
{
    free(v->items);
    v->capacity = 0;
    v->size = 0;
}
