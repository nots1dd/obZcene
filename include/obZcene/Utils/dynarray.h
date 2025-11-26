#ifndef OBZ_UTILS_DYNARRAY_H
#define OBZ_UTILS_DYNARRAY_H

#include <stdlib.h>
#include <string.h>

typedef size_t arr_size;

typedef struct
{
  void*    obz_data;
  arr_size size;
  size_t   capacity;
  size_t   elem_size;
} OBZ_DynArray;

inline static OBZ_DynArray obz_arr_create(size_t elem_size)
{
  OBZ_DynArray a;
  a.obz_data  = NULL;
  a.size      = 0;
  a.capacity  = 0;
  a.elem_size = elem_size;
  return a;
}

inline static void obz_arr_free(OBZ_DynArray* a)
{
  free(a->obz_data);
  a->obz_data = NULL;
  a->size     = 0;
  a->capacity = 0;
}

inline static size_t obz_arr_size(const OBZ_DynArray* a) { return a->size; }

inline static void obz_arr_reserve(OBZ_DynArray* a, size_t new_cap)
{
  if (new_cap > a->capacity)
  {
    void* p = realloc(a->obz_data, new_cap * a->elem_size);
    if (!p)
      abort();
    a->obz_data = p;
    a->capacity = new_cap;
  }
}

inline static void obz_arr_push(OBZ_DynArray* a, const void* elem)
{
  if (a->size == a->capacity)
    obz_arr_reserve(a, a->capacity ? a->capacity * 2 : 8);
  memcpy((char*)a->obz_data + a->size * a->elem_size, elem, a->elem_size);
  a->size++;
}

inline static void obz_arr_pop(OBZ_DynArray* a, void* out_elem)
{
  if (a->size == 0)
    return;
  a->size--;
  if (out_elem)
    memcpy(out_elem, (char*)a->obz_data + a->size * a->elem_size, a->elem_size);
}

#endif
