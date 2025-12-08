#ifndef OBZCENE_UTILS_ARENA_ALLOCATOR_H
#define OBZCENE_UTILS_ARENA_ALLOCATOR_H

#include "obz_macros.h"
#include <stdint.h>
#include <stdlib.h>

OBZ_BEGIN_CPP_DECLS

// works for vec3, vec2, etc
//
// -> primitive dtypes should be aligned to their size (ex: float = 4 bytes)
//
// -> structs should be aligned to max member size (ex: struct { float x; float y; } = 4 bytes)
#define OBZ_ARENA_DEFAULT_ALIGNMENT 16

/*
 * Arena allocator:
 * - One malloc for both the arena struct AND its memory block
 * - Supports alignment for fast SIMD / float math
 */

typedef struct
{
  uint8_t* base; // start of memory block
  size_t   capacity;
  size_t   offset;
} OBZ_Arena;

/*
 * Create arena with capacity = cap bytes.
 * Allocates:
 *    [OBZ_Arena struct][arena memory...]
 */
OBZ_API_INLINE static OBZ_Arena* obz_arena_init(size_t cap)
{
  size_t     total = sizeof(OBZ_Arena) + cap;
  OBZ_Arena* a     = (OBZ_Arena*)malloc(total);
  if (!a)
    return NULL;

  a->base     = (uint8_t*)(a + 1); // block starts after struct
  a->capacity = cap;
  a->offset   = 0;

  return a;
}

OBZ_API_INLINE static void obz_arena_reset(OBZ_Arena* a) { a->offset = 0; }

/*
 * Allocate aligned memory from the arena.
 *
 * align MUST be a power of two!!
 */
OBZ_API_INLINE static void* obz_arena_alloc(OBZ_Arena* a, size_t size, size_t align)
{

  OBZ_ASSERT((align & (align - 1)) == 0, "Alignment must be a power of two.");

  size_t aligned = (a->offset + align - 1) & ~(align - 1);

  if (aligned + size > a->capacity)
    return NULL;

  void* ptr = a->base + aligned;
  a->offset = aligned + size;

  return ptr;
}

OBZ_END_CPP_DECLS

#endif
