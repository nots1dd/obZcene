#ifndef OBZ_UTILS_DYNARRAY_H
#define OBZ_UTILS_DYNARRAY_H

#include "obz_macros.h"
#include "obz_mem.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// Dynamic array utility (like std::vector in C++)
//
// header only so all API is decl'd as OBZ_API_INLINE static.

OBZ_BEGIN_CPP_DECLS

typedef size_t obz_index_t; /* index into the array    */
typedef size_t obz_count_t; /* number of elements      */
typedef size_t obz_bytes_t; /* number of bytes         */

/*
    elem_size  = size in bytes of a single element
    size       = number of valid elements
    capacity   = number of allocated slots (in elements)
  */
typedef struct
{
  void*       obz_data;
  obz_count_t size;
  obz_count_t capacity;
  obz_bytes_t elem_size;
} OBZ_DynArray;

/* ----- Create empty dynamic array (no allocation yet) ----- */
OBZ_API_INLINE static OBZ_DynArray obz_arr_create(obz_bytes_t elem_size)
{
  OBZ_DynArray a;
  a.obz_data  = NULL;
  a.size      = 0;
  a.capacity  = 0;
  a.elem_size = elem_size;
  return a;
}

/* ----- Free array ----- */
OBZ_API_INLINE static void obz_arr_free(OBZ_DynArray* a)
{
  free(a->obz_data);
  a->obz_data = NULL;
  a->size     = 0;
  a->capacity = 0;
}

/* ----- Length (element count) ----- */
OBZ_API_INLINE static obz_count_t obz_arr_size(const OBZ_DynArray* a) { return a->size; }

/* ----- Ensure capacity for new_cap elements ----- */
OBZ_API_INLINE static void obz_arr_reserve(OBZ_DynArray* a, obz_count_t new_cap)
{
  if (new_cap > a->capacity)
  {
    void* p     = obz_realloc(a->obz_data, (obz_bytes_t)new_cap * a->elem_size);
    a->obz_data = p;
    a->capacity = new_cap;
  }
}

/* ----- Push element to end ----- */
OBZ_API_INLINE static void obz_arr_push(OBZ_DynArray* a, const void* elem)
{
  if (a->size == a->capacity)
    obz_arr_reserve(a, a->capacity ? a->capacity * 2 : 8);

  memcpy((char*)a->obz_data + a->size * a->elem_size, elem, a->elem_size);

  a->size++;
}

/* ----- Pop element from end ----- */
OBZ_API_INLINE static void obz_arr_pop(OBZ_DynArray* a, void* out_elem)
{
  if (a->size == 0)
    return;

  a->size--;

  if (out_elem)
    memcpy(out_elem, (char*)a->obz_data + a->size * a->elem_size, a->elem_size);
}

/* ----- Get element pointer ----- */
OBZ_API_INLINE static void* obz_arr_get(OBZ_DynArray* a, obz_index_t idx)
{
  if (idx >= a->size)
    return NULL;
  return (char*)a->obz_data + idx * a->elem_size;
}

/* ----- Const version ----- */
OBZ_API_INLINE static const void* obz_arr_get_const(const OBZ_DynArray* a, obz_index_t idx)
{
  if (idx >= a->size)
    return NULL;
  return (const char*)a->obz_data + idx * a->elem_size;
}

/* ----- Get pointer to the entire array data (read-only) ----- */
OBZ_API_INLINE static const void* obz_arr_get_data_const(const OBZ_DynArray* a)
{
  if (!a || !a->obz_data || a->size == 0)
    return NULL;
  return a->obz_data;
}

/* ----- Get typed pointer to entire array (read/write) ----- */
#define obz_arr_get_data(a, T) ((T*)((a) && (a)->obz_data ? (a)->obz_data : NULL))

/* ----- Set element at index ----- */
OBZ_API_INLINE static void obz_arr_set(OBZ_DynArray* a, obz_index_t idx, const void* elem)
{
  if (idx >= a->size)
    return;

  memcpy((char*)a->obz_data + idx * a->elem_size, elem, a->elem_size);
}

/* ----- Check whether initialized + contains elements ----- */
OBZ_API_INLINE static bool obz_arr_exists(const OBZ_DynArray* array)
{
  return array && array->obz_data != NULL && array->size > 0;
}

OBZ_END_CPP_DECLS

#endif /* OBZ_UTILS_DYNARRAY_H */
