#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void *Kmalloc(size_t size);
void *Krealloc(void *ptr, size_t size);
void  Kfree(void *ptr);

#ifdef __cplusplus
}
#endif

static inline void *noodle_malloc(size_t size)
{
    return Kmalloc(size);
}

static inline void *noodle_realloc(void *ptr, size_t size)
{
    return Krealloc(ptr, size);
}

static inline void noodle_free(void *ptr)
{
    Kfree(ptr);
}