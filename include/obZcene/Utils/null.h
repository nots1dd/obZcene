#ifndef OBZ_UTILS_NULL_H
#define OBZ_UTILS_NULL_H

#include "obz_macros.h"
#include <stddef.h>

/*------------------------------------------------------------
    Basic NULL checks
  ------------------------------------------------------------*/

/* Returns NULL if ptr is NULL, otherwise returns ptr. */
OBZ_INTERNAL_INLINE static void* __OBZ_null_or_self(void* ptr) { return ptr ? ptr : NULL; }

/* Same as above, but typed via macro. */
#define __OBZ_NULL_OR_SELF(p) ((p) ? (p) : NULL)

/* Returns 1 if ptr is NULL, 0 otherwise. */
OBZ_INTERNAL_INLINE static int __OBZ_is_null(const void* ptr) { return ptr == NULL; }

/* Returns 1 if ptr is NOT NULL, 0 otherwise. */
OBZ_INTERNAL_INLINE static int __OBZ_is_not_null(const void* ptr) { return ptr != NULL; }

/*------------------------------------------------------------
    Chained / helper patterns
  ------------------------------------------------------------*/

/* If ptr is NULL, return fallback; else return ptr. */
OBZ_INTERNAL_INLINE static void* __OBZ_null_or_fallback(void* ptr, void* fallback)
{
  return ptr ? ptr : fallback;
}

#define __OBZ_NULL_OR_FALLBACK(p, fb) ((p) ? (p) : (fb))

/* Execute an expression only if ptr is not NULL. */
#define __OBZ_DO_IF_NOT_NULL(p, expr) \
  do                                  \
  {                                   \
    if ((p) != NULL)                  \
    {                                 \
      expr;                           \
    }                                 \
  } while (0)

/* Same, but return a value if NULL. */
#define __OBZ_RETURN_IF_NULL(p, ret) \
  do                                 \
  {                                  \
    if ((p) == NULL)                 \
      return (ret);                  \
  } while (0)

#define __OBZ_RETURN_NULL_IF_NULL(p) __OBZ_RETURN_IF_NULL(p, NULL)

#define __OBZ_RETURN_VOID_IF_NULL(p) \
  do                                 \
  {                                  \
    if ((p) == NULL)                 \
      return;                        \
  } while (0)

/*------------------------------------------------------------
    Casting helpers
  ------------------------------------------------------------*/

/* Safe cast: returns NULL if ptr is NULL, else casted. */
#define __OBZ_CAST_IF_NOT_NULL(type, p) ((p) ? (type)(p) : NULL)

/*------------------------------------------------------------
    Arrays / counts
  ------------------------------------------------------------*/

/* Returns 0 if array pointer is NULL; useful for dynamic arrays. */
#define __OBZ_LEN_IF_NOT_NULL(arr, len) ((arr) ? (len) : 0)

#endif /* OBZ_UTILS_NULL_H */
