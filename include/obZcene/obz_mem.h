#ifndef OBZ_MEMORY_H
#define OBZ_MEMORY_H

#include "obz_log.h"
#include "obz_macros.h"
#include <stdio.h>
#include <stdlib.h>

// NOTE THAT IF ALLOCATION FAILS IT WILL ABORT SO NO NEED TO WRITE LOGIC FOR NULL CHECKS

OBZ_BEGIN_CPP_DECLS

static inline void* obz_malloc(size_t size)
{
  void* ptr = malloc(size);
  if (!ptr && size != 0)
  {
    OBZ_LOG_ERROR(NULL, "obz_malloc failed: requested %zu bytes", size);
    // can put more graceful handling here if needed
    abort();
  }
  return ptr;
}

static inline void* obz_calloc(size_t nmemb, size_t size)
{
  void* ptr = calloc(nmemb, size);
  if (!ptr && nmemb != 0 && size != 0)
  {
    OBZ_LOG_ERROR(NULL, "obz_calloc failed: requested %zu elements of %zu bytes", nmemb, size);
    abort();
  }
  return ptr;
}

static inline void* obz_realloc(void* ptr, size_t new_size)
{
  void* new_ptr = realloc(ptr, new_size);
  if (!new_ptr && new_size != 0)
  {
    OBZ_LOG_ERROR(NULL, "obz_realloc failed: requested %zu bytes", new_size);
    abort();
  }
  return new_ptr;
}

#define OBZ_STACK_ALLOC(nmemb, size) alloca(nmemb* size)

static inline void __obz_free_impl(void** pp)
{
  if (pp && *pp)
  {
    free(*pp);
    *pp = NULL;
  }
}

#define obz_free(ptr)         \
  _Generic((ptr),             \
    void*: __obz_free_impl,   \
    char*: __obz_free_impl,   \
    int*: __obz_free_impl,    \
    float*: __obz_free_impl,  \
    double*: __obz_free_impl, \
    default: __obz_free_impl)((void**)&(ptr))

OBZ_END_CPP_DECLS

#endif /* OBZ_MEMORY_H */
