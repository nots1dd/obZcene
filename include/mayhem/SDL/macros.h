#ifndef SDL_API_MACROS_H
#define SDL_API_MACROS_H

/* Force-inline helper */
#if defined(__GNUC__) || defined(__clang__)
#define MHMAPI_INLINE __attribute__((always_inline)) inline
#else
#define MHMAPI_INLINE inline
#endif

#endif
