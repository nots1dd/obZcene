#ifndef OBZ_SDL_API_MACROS_H
#define OBZ_SDL_API_MACROS_H

/* Force-inline helper */
#if defined(__GNUC__) || defined(__clang__)
#define OBZ_FORCE_INLINE __attribute__((always_inline)) inline
#else
#define OBZ_FORCE_INLINE inline
#endif

#endif
