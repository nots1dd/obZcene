#ifndef OBZ_CONFIG_H
#define OBZ_CONFIG_H

#include <stdint.h>

/* ------------------------------------------------------------
   OS DETECTION
   ------------------------------------------------------------ */
#if defined(_WIN32) || defined(_WIN64)
#define OBZ_OS_WINDOWS 1
#else
#define OBZ_OS_WINDOWS 0
#endif

#if defined(__linux__)
#define OBZ_OS_LINUX 1
#else
#define OBZ_OS_LINUX 0
#endif

#if defined(__APPLE__) && defined(__MACH__)
#define OBZ_OS_MACOS 1
#else
#define OBZ_OS_MACOS 0
#endif

#if defined(__FreeBSD__)
#define OBZ_OS_FREEBSD 1
#else
#define OBZ_OS_FREEBSD 0
#endif

/* ------------------------------------------------------------
   COMPILER DETECTION
   ------------------------------------------------------------ */
#if defined(__clang__)
#define OBZ_COMPILER_CLANG 1
#define OBZ_COMPILER_GCC   0
#define OBZ_COMPILER_MSVC  0
#define OBZ_CLANG_VERSION  __clang_major__
#elif defined(__GNUC__)
#define OBZ_COMPILER_CLANG 0
#define OBZ_COMPILER_GCC   1
#define OBZ_COMPILER_MSVC  0
#define OBZ_GCC_VERSION    __GNUC__
#elif defined(_MSC_VER)
#define OBZ_COMPILER_CLANG 0
#define OBZ_COMPILER_GCC   0
#define OBZ_COMPILER_MSVC  1
#define OBZ_MSVC_VERSION   _MSC_VER
#else
#error "Unknown compiler!"
#endif

/* ------------------------------------------------------------
   ARCHITECTURE DETECTION
   ------------------------------------------------------------ */
#if defined(__x86_64__) || defined(_M_X64)
#define OBZ_ARCH_X86_64 1
#else
#define OBZ_ARCH_X86_64 0
#endif

#if defined(__i386__) || defined(_M_IX86)
#define OBZ_ARCH_X86_32 1
#else
#define OBZ_ARCH_X86_32 0
#endif

#if defined(__aarch64__)
#define OBZ_ARCH_ARM64 1
#else
#define OBZ_ARCH_ARM64 0
#endif

#if defined(__arm__) || defined(_M_ARM)
#define OBZ_ARCH_ARM32 1
#else
#define OBZ_ARCH_ARM32 0
#endif

#if defined(__riscv) || defined(__riscv__)
#define OBZ_ARCH_RISCV 1
#else
#define OBZ_ARCH_RISCV 0
#endif

#if defined(__powerpc64__)
#define OBZ_ARCH_PPC64 1
#else
#define OBZ_ARCH_PPC64 0
#endif

/* ------------------------------------------------------------
   SIMD / INTRINSICS DETECTION (compile-time)
   ------------------------------------------------------------ */

/* ---------------------- x86 SSE family ---------------------- */
#if OBZ_ARCH_X86_32 || OBZ_ARCH_X86_64

#if defined(__MMX__) || (defined(_M_IX86_FP) && _M_IX86_FP >= 0)
#define OBZ_HAS_MMX 1
#else
#define OBZ_HAS_MMX 0
#endif

#if defined(__SSE__) || (defined(_M_IX86_FP) && _M_IX86_FP >= 1)
#define OBZ_HAS_SSE 1
#else
#define OBZ_HAS_SSE 0
#endif

#if defined(__SSE2__) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#define OBZ_HAS_SSE2 1
#else
#define OBZ_HAS_SSE2 0
#endif

#if defined(__SSE3__)
#define OBZ_HAS_SSE3 1
#else
#define OBZ_HAS_SSE3 0
#endif

#if defined(__SSSE3__)
#define OBZ_HAS_SSSE3 1
#else
#define OBZ_HAS_SSSE3 0
#endif

#if defined(__SSE4_1__)
#define OBZ_HAS_SSE41 1
#else
#define OBZ_HAS_SSE41 0
#endif

#if defined(__SSE4_2__)
#define OBZ_HAS_SSE42 1
#else
#define OBZ_HAS_SSE42 0
#endif

#if defined(__AVX__)
#define OBZ_HAS_AVX 1
#else
#define OBZ_HAS_AVX 0
#endif

#if defined(__AVX2__)
#define OBZ_HAS_AVX2 1
#else
#define OBZ_HAS_AVX2 0
#endif

#if defined(__AVX512F__)
#define OBZ_HAS_AVX512 1
#else
#define OBZ_HAS_AVX512 0
#endif

#else
#define OBZ_HAS_MMX    0
#define OBZ_HAS_SSE    0
#define OBZ_HAS_SSE2   0
#define OBZ_HAS_SSE3   0
#define OBZ_HAS_SSSE3  0
#define OBZ_HAS_SSE41  0
#define OBZ_HAS_SSE42  0
#define OBZ_HAS_AVX    0
#define OBZ_HAS_AVX2   0
#define OBZ_HAS_AVX512 0
#endif

/* ---------------------- ARM NEON / SVE ---------------------- */
#if OBZ_ARCH_ARM32 || OBZ_ARCH_ARM64

#if defined(__ARM_NEON__) || defined(__ARM_NEON)
#define OBZ_HAS_NEON 1
#else
#define OBZ_HAS_NEON 0
#endif

#if defined(__ARM_FEATURE_FMA)
#define OBZ_HAS_ARM_FMA 1
#else
#define OBZ_HAS_ARM_FMA 0
#endif

#if defined(__ARM_FEATURE_CRYPTO)
#define OBZ_HAS_ARM_CRYPTO 1
#else
#define OBZ_HAS_ARM_CRYPTO 0
#endif

#if defined(__ARM_FEATURE_SVE)
#define OBZ_HAS_SVE 1
#else
#define OBZ_HAS_SVE 0
#endif

#else
#define OBZ_HAS_NEON       0
#define OBZ_HAS_ARM_FMA    0
#define OBZ_HAS_ARM_CRYPTO 0
#define OBZ_HAS_SVE        0
#endif

/* ---------------------- RISC-V Vector ---------------------- */
#if OBZ_ARCH_RISCV
#if defined(__riscv_vector)
#define OBZ_HAS_RISCV_VECTOR 1
#else
#define OBZ_HAS_RISCV_VECTOR 0
#endif
#else
#define OBZ_HAS_RISCV_VECTOR 0
#endif

/* -------------------- SIMD Description -------------------- */
#define OBZ_SIMD_DESC_BASE ""

#if OBZ_HAS_SSE2
#define OBZ_SIMD_DESC_SSE2 "SSE2 "
#else
#define OBZ_SIMD_DESC_SSE2 ""
#endif

#if OBZ_HAS_SSE3
#define OBZ_SIMD_DESC_SSE3 "SSE3 "
#else
#define OBZ_SIMD_DESC_SSE3 ""
#endif

#if OBZ_HAS_SSSE3
#define OBZ_SIMD_DESC_SSSE3 "SSSE3 "
#else
#define OBZ_SIMD_DESC_SSSE3 ""
#endif

#if OBZ_HAS_SSE41
#define OBZ_SIMD_DESC_SSE41 "SSE4.1 "
#else
#define OBZ_SIMD_DESC_SSE41 ""
#endif

#if OBZ_HAS_SSE42
#define OBZ_SIMD_DESC_SSE42 "SSE4.2 "
#else
#define OBZ_SIMD_DESC_SSE42 ""
#endif

#if OBZ_HAS_AVX
#define OBZ_SIMD_DESC_AVX "AVX "
#else
#define OBZ_SIMD_DESC_AVX ""
#endif

#if OBZ_HAS_AVX2
#define OBZ_SIMD_DESC_AVX2 "AVX2 "
#else
#define OBZ_SIMD_DESC_AVX2 ""
#endif

#if OBZ_HAS_AVX512
#define OBZ_SIMD_DESC_AVX512 "AVX512 "
#else
#define OBZ_SIMD_DESC_AVX512 ""
#endif

#if OBZ_HAS_NEON
#define OBZ_SIMD_DESC_NEON "NEON "
#else
#define OBZ_SIMD_DESC_NEON ""
#endif

#if OBZ_HAS_RISCV_VECTOR
#define OBZ_SIMD_DESC_RV "RISC-V-Vector "
#else
#define OBZ_SIMD_DESC_RV ""
#endif

#define OBZ_SIMD_DESC  \
  OBZ_SIMD_DESC_SSE2   \
  OBZ_SIMD_DESC_SSE3   \
  OBZ_SIMD_DESC_SSSE3  \
  OBZ_SIMD_DESC_SSE41  \
  OBZ_SIMD_DESC_SSE42  \
  OBZ_SIMD_DESC_AVX    \
  OBZ_SIMD_DESC_AVX2   \
  OBZ_SIMD_DESC_AVX512 \
  OBZ_SIMD_DESC_NEON   \
  OBZ_SIMD_DESC_RV

#endif /* OBZ_CONFIG_H */
