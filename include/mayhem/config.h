#ifndef MHM_CONFIG_H
#define MHM_CONFIG_H

#include <stdint.h>

/* ------------------------------------------------------------
   OS DETECTION
   ------------------------------------------------------------ */
#if defined(_WIN32) || defined(_WIN64)
#   define MHM_OS_WINDOWS 1
#else
#   define MHM_OS_WINDOWS 0
#endif

#if defined(__linux__)
#   define MHM_OS_LINUX 1
#else
#   define MHM_OS_LINUX 0
#endif

#if defined(__APPLE__) && defined(__MACH__)
#   define MHM_OS_MACOS 1
#else
#   define MHM_OS_MACOS 0
#endif

#if defined(__FreeBSD__)
#   define MHM_OS_FREEBSD 1
#else
#   define MHM_OS_FREEBSD 0
#endif

/* ------------------------------------------------------------
   COMPILER DETECTION
   ------------------------------------------------------------ */
#if defined(__clang__)
#   define MHM_COMPILER_CLANG 1
#   define MHM_COMPILER_GCC   0
#   define MHM_COMPILER_MSVC  0
#   define MHM_CLANG_VERSION  __clang_major__
#elif defined(__GNUC__)
#   define MHM_COMPILER_CLANG 0
#   define MHM_COMPILER_GCC   1
#   define MHM_COMPILER_MSVC  0
#   define MHM_GCC_VERSION    __GNUC__
#elif defined(_MSC_VER)
#   define MHM_COMPILER_CLANG 0
#   define MHM_COMPILER_GCC   0
#   define MHM_COMPILER_MSVC  1
#   define MHM_MSVC_VERSION   _MSC_VER
#else
#   error "Unknown compiler!"
#endif

/* ------------------------------------------------------------
   ARCHITECTURE DETECTION
   ------------------------------------------------------------ */
#if defined(__x86_64__) || defined(_M_X64)
#   define MHM_ARCH_X86_64 1
#else
#   define MHM_ARCH_X86_64 0
#endif

#if defined(__i386__) || defined(_M_IX86)
#   define MHM_ARCH_X86_32 1
#else
#   define MHM_ARCH_X86_32 0
#endif

#if defined(__aarch64__)
#   define MHM_ARCH_ARM64 1
#else
#   define MHM_ARCH_ARM64 0
#endif

#if defined(__arm__) || defined(_M_ARM)
#   define MHM_ARCH_ARM32 1
#else
#   define MHM_ARCH_ARM32 0
#endif

#if defined(__riscv) || defined(__riscv__)
#   define MHM_ARCH_RISCV 1
#else
#   define MHM_ARCH_RISCV 0
#endif

#if defined(__powerpc64__)
#   define MHM_ARCH_PPC64 1
#else
#   define MHM_ARCH_PPC64 0
#endif

/* ------------------------------------------------------------
   SIMD / INTRINSICS DETECTION (compile-time)
   ------------------------------------------------------------ */

/* ---------------------- x86 SSE family ---------------------- */
#if MHM_ARCH_X86_32 || MHM_ARCH_X86_64

# if defined(__MMX__) || (defined(_M_IX86_FP) && _M_IX86_FP >= 0)
#   define MHM_HAS_MMX 1
# else
#   define MHM_HAS_MMX 0
# endif

# if defined(__SSE__) || (defined(_M_IX86_FP) && _M_IX86_FP >= 1)
#   define MHM_HAS_SSE 1
# else
#   define MHM_HAS_SSE 0
# endif

# if defined(__SSE2__) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#   define MHM_HAS_SSE2 1
# else
#   define MHM_HAS_SSE2 0
# endif

# if defined(__SSE3__)
#   define MHM_HAS_SSE3 1
# else
#   define MHM_HAS_SSE3 0
# endif

# if defined(__SSSE3__)
#   define MHM_HAS_SSSE3 1
# else
#   define MHM_HAS_SSSE3 0
# endif

# if defined(__SSE4_1__)
#   define MHM_HAS_SSE41 1
# else
#   define MHM_HAS_SSE41 0
# endif

# if defined(__SSE4_2__)
#   define MHM_HAS_SSE42 1
# else
#   define MHM_HAS_SSE42 0
# endif

# if defined(__AVX__)
#   define MHM_HAS_AVX 1
# else
#   define MHM_HAS_AVX 0
# endif

# if defined(__AVX2__)
#   define MHM_HAS_AVX2 1
# else
#   define MHM_HAS_AVX2 0
# endif

# if defined(__AVX512F__)
#   define MHM_HAS_AVX512 1
# else
#   define MHM_HAS_AVX512 0
# endif

#else
# define MHM_HAS_MMX 0
# define MHM_HAS_SSE 0
# define MHM_HAS_SSE2 0
# define MHM_HAS_SSE3 0
# define MHM_HAS_SSSE3 0
# define MHM_HAS_SSE41 0
# define MHM_HAS_SSE42 0
# define MHM_HAS_AVX 0
# define MHM_HAS_AVX2 0
# define MHM_HAS_AVX512 0
#endif

/* ---------------------- ARM NEON / SVE ---------------------- */
#if MHM_ARCH_ARM32 || MHM_ARCH_ARM64

# if defined(__ARM_NEON__) || defined(__ARM_NEON)
#   define MHM_HAS_NEON 1
# else
#   define MHM_HAS_NEON 0
# endif

# if defined(__ARM_FEATURE_FMA)
#   define MHM_HAS_ARM_FMA 1
# else
#   define MHM_HAS_ARM_FMA 0
# endif

# if defined(__ARM_FEATURE_CRYPTO)
#   define MHM_HAS_ARM_CRYPTO 1
# else
#   define MHM_HAS_ARM_CRYPTO 0
# endif

# if defined(__ARM_FEATURE_SVE)
#   define MHM_HAS_SVE 1
# else
#   define MHM_HAS_SVE 0
# endif

#else
# define MHM_HAS_NEON 0
# define MHM_HAS_ARM_FMA 0
# define MHM_HAS_ARM_CRYPTO 0
# define MHM_HAS_SVE 0
#endif

/* ---------------------- RISC-V Vector ---------------------- */
#if MHM_ARCH_RISCV
# if defined(__riscv_vector)
#   define MHM_HAS_RISCV_VECTOR 1
# else
#   define MHM_HAS_RISCV_VECTOR 0
# endif
#else
# define MHM_HAS_RISCV_VECTOR 0
#endif

/* -------------------- SIMD Description -------------------- */
#define MHM_SIMD_DESC_BASE ""

#if MHM_HAS_SSE2
# define MHM_SIMD_DESC_SSE2 "SSE2 "
#else
# define MHM_SIMD_DESC_SSE2 ""
#endif

#if MHM_HAS_SSE3
# define MHM_SIMD_DESC_SSE3 "SSE3 "
#else
# define MHM_SIMD_DESC_SSE3 ""
#endif

#if MHM_HAS_SSSE3
# define MHM_SIMD_DESC_SSSE3 "SSSE3 "
#else
# define MHM_SIMD_DESC_SSSE3 ""
#endif

#if MHM_HAS_SSE41
# define MHM_SIMD_DESC_SSE41 "SSE4.1 "
#else
# define MHM_SIMD_DESC_SSE41 ""
#endif

#if MHM_HAS_SSE42
# define MHM_SIMD_DESC_SSE42 "SSE4.2 "
#else
# define MHM_SIMD_DESC_SSE42 ""
#endif

#if MHM_HAS_AVX
# define MHM_SIMD_DESC_AVX "AVX "
#else
# define MHM_SIMD_DESC_AVX ""
#endif

#if MHM_HAS_AVX2
# define MHM_SIMD_DESC_AVX2 "AVX2 "
#else
# define MHM_SIMD_DESC_AVX2 ""
#endif

#if MHM_HAS_AVX512
# define MHM_SIMD_DESC_AVX512 "AVX512 "
#else
# define MHM_SIMD_DESC_AVX512 ""
#endif

#if MHM_HAS_NEON
# define MHM_SIMD_DESC_NEON "NEON "
#else
# define MHM_SIMD_DESC_NEON ""
#endif

#if MHM_HAS_RISCV_VECTOR
# define MHM_SIMD_DESC_RV "RISC-V-Vector "
#else
# define MHM_SIMD_DESC_RV ""
#endif

#define MHM_SIMD_DESC \
    MHM_SIMD_DESC_SSE2 \
    MHM_SIMD_DESC_SSE3 \
    MHM_SIMD_DESC_SSSE3 \
    MHM_SIMD_DESC_SSE41 \
    MHM_SIMD_DESC_SSE42 \
    MHM_SIMD_DESC_AVX \
    MHM_SIMD_DESC_AVX2 \
    MHM_SIMD_DESC_AVX512 \
    MHM_SIMD_DESC_NEON \
    MHM_SIMD_DESC_RV

#endif /* MHM_CONFIG_H */
