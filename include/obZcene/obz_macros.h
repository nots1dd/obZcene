#ifndef OBZ_API_MACROS_H
#define OBZ_API_MACROS_H

/* =========================================================================
   FORCE INLINE / NOINLINE
   ========================================================================= */
#if defined(__GNUC__) || defined(__clang__)
#define OBZ_FORCE_INLINE __attribute__((always_inline)) inline
#define OBZ_NO_INLINE    __attribute__((noinline))
#else
#define OBZ_FORCE_INLINE inline
#define OBZ_NO_INLINE
#endif

// CPP DECL

#ifdef __cplusplus
#define OBZ_BEGIN_CPP_DECLS \
  extern "C"                \
  {
#define OBZ_END_CPP_DECLS }
#else
#define OBZ_BEGIN_CPP_DECLS
#define OBZ_END_CPP_DECLS
#endif

#define OBZ_STRINGIFY_(x) #x
#define OBZ_STRINGIFY(x)  OBZ_STRINGIFY_(x)

#define OBZ_DO_PRAGMA(x) _Pragma(#x)

/* =========================================================================
   DEPRECATED
   ========================================================================= */
#if defined(__GNUC__) || defined(__clang__)
#define OBZ_API_DEPRECATED(msg) __attribute__((deprecated(msg)))
#elif defined(_MSC_VER)
#define OBZ_API_DEPRECATED(msg) __declspec(deprecated(msg))
#else
#define OBZ_API_DEPRECATED(msg)
#endif

/* =========================================================================
   DEPRECATED
   ========================================================================= */
#if defined(__GNUC__) || defined(__clang__)
#define OBZ_API_REMOVED(msg) __attribute__((unavailable(msg)))
#elif defined(_MSC_VER)
#define OBZ_API_REMOVED(msg)  \
  __declspec(deprecated(msg)) \
  __declspec(selectany) extern int OBZ_REMOVED_API_##__LINE__##_ERROR__[-1]
#else
#define OBZ_API_REMOVED(msg) OBZ_DO_PRAGMA(GCC error "This API is removed")
#endif

/* =========================================================================
   LIKELY / UNLIKELY (branch prediction)
   ========================================================================= */
#if defined(__GNUC__) || defined(__clang__)
#define OBZ_LIKELY(x)   __builtin_expect(!!(x), 1)
#define OBZ_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define OBZ_LIKELY(x)   (x)
#define OBZ_UNLIKELY(x) (x)
#endif

/* =========================================================================
   ARRAY SIZE
   ========================================================================= */
#define OBZ_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/* =========================================================================
   STATIC ASSERT
   ========================================================================= */
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#define OBZ_STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)
#else
#define OBZ_STATIC_ASSERT(cond, msg) typedef char static_assert_##msg[(cond) ? 1 : -1]
#endif

/* =========================================================================
   UNUSED / MAYBE_UNUSED
   ========================================================================= */
#if defined(__GNUC__) || defined(__clang__)
#define OBZ_UNUSED       __attribute__((unused))
#define OBZ_MAYBE_UNUSED __attribute__((unused))
#else
#define OBZ_UNUSED
#define OBZ_MAYBE_UNUSED
#endif

/* =========================================================================
   API EXPORT / IMPORT
   ========================================================================= */
#if defined(_WIN32) || defined(_WIN64)
#ifdef OBZ_EXPORT
#define OBZ_API __declspec(dllexport)
#else
#define OBZ_API __declspec(dllimport)
#endif
#else
#define OBZ_API __attribute__((visibility("default")))
#endif

/* Public API tag for documentation purposes */
#define OBZ_API_INLINE OBZ_FORCE_INLINE
#define OBZ_INTERNAL   OBZ_NO_INLINE

/* =========================================================================
   CONSTEXPR / CONST
   ========================================================================= */
#if defined(__cplusplus)
#define OBZ_CONSTEXPR constexpr
#else
#define OBZ_CONSTEXPR const
#endif

/* =========================================================================
    PACKED STRUCT MACRO: define a packed struct
   ========================================================================= */
#if defined(_MSC_VER)
#define OBZ_PACKED_STRUCT(name) __pragma(pack(push, 1)) struct name __pragma(pack(pop))
#else
#define OBZ_PACKED_STRUCT(name) struct __attribute__((packed)) name
#endif

/* =========================================================================
   ALIGNMENT (for SIMD / cache optimization) [dont know if this works]
   ========================================================================= */
#if defined(_MSC_VER)
#define OBZ_ALIGN(x) __declspec(align(x))
#else
#define OBZ_ALIGN(x) __attribute__((aligned(x)))
#endif

/* =========================================================================
   CACHE LINE / PREFETCH HINTS
   ========================================================================= */
#define OBZ_CACHE_LINE_SIZE 64
#if defined(__GNUC__) || defined(__clang__)
#define OBZ_PREFETCH(addr, rw, locality) __builtin_prefetch((addr), (rw), (locality))
#else
#define OBZ_PREFETCH(addr, rw, locality)
#endif

/* =========================================================================
   THREAD LOCAL STORAGE
   ========================================================================= */
#if defined(_MSC_VER)
#define OBZ_THREAD_LOCAL __declspec(thread)
#elif defined(__GNUC__) || defined(__clang__)
#define OBZ_THREAD_LOCAL __thread
#else
#define OBZ_THREAD_LOCAL _Thread_local
#endif

/* =========================================================================
   SIMD / VECTOR FRIENDLY
   ========================================================================= */
#if defined(__GNUC__) || defined(__clang__)
#define OBZ_SIMD_ALIGN(x) __attribute__((vector_size(x)))
#else
#define OBZ_SIMD_ALIGN(x)
#endif

/* =========================================================================
   GRAPHICS / ENGINE SPECIFIC (may add more idk)
   ========================================================================= */

#define OBZ_ONE_RAD_IN_DEG 0.017453292519943295f
#define OBZ_ONE_DEG_IN_RAD 57.29577951308232f

/* Converts degrees to radians */
#define OBZ_DEG2RAD(d) ((d) * OBZ_ONE_RAD_IN_DEG)

/* Converts radians to degrees */
#define OBZ_RAD2DEG(r) ((r) * OBZ_ONE_DEG_IN_RAD)

/* =========================================================================
   DEBUG / ASSERTIONS
   ========================================================================= */
#include <assert.h>
#include <stdio.h>
#define OBZ_ASSERT(cond, msg)                         \
  do                                                  \
  {                                                   \
    if (!(cond))                                      \
    {                                                 \
      fprintf(stderr, "Assertion failed: %s\n", msg); \
      assert(cond);                                   \
    }                                                 \
  } while (0)
#define OBZ_STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)

#endif /* OBZ_API_MACROS_H */
