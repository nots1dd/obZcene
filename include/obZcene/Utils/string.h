#ifndef OBZ_UTILS_STRING_H
#define OBZ_UTILS_STRING_H

#include "obz_mem.h"
#include "obz_types.h"
#include <ctype.h>
#include <string.h>

OBZ_BEGIN_CPP_DECLS

/* Extract directory portion of a path (returns a malloc'd string, caller must free).
   If no directory, returns strdup(".") */
inline static char* __OBZ_dirname_alloc(const char* path)
{
  if (!path)
    return strdup(".");
  const char* last_slash = strrchr(path, '/');

  if (!last_slash)
    return strdup(".");

  size_t len = (size_t)(last_slash - path);
  char*  out = (char*)obz_malloc(len + 1);

  memcpy(out, path, len);
  out[len] = '\0';
  return out;
}

static inline void obz_strncpy(char* dst, const char* src, size_t dst_size)
{
  if (!dst || !src || dst_size == 0)
    return;

  size_t n = strnlen(src, dst_size - 1);
  memcpy(dst, src, n);
  dst[n] = '\0';
}

inline static char* __OBZ_str_concat(const char* a, const char* b)
{
  if (!a || !b)
    return NULL;

  size_t la  = strlen(a);
  size_t lb  = strlen(b);
  char*  out = (char*)obz_malloc(la + lb + 1);

  memcpy(out, a, la);
  memcpy(out + la, b, lb);
  out[la + lb] = '\0';

  return out;
}

/* join dir + filename into a malloc'd path (caller frees) */
inline static char* __OBZ_join_path(const char* dir, const char* file)
{
  if (!dir || !file)
    return NULL;

  size_t ld       = strlen(dir);
  int    need_sep = (ld > 0 && dir[ld - 1] != '/' && dir[ld - 1] != '\\');

  if (need_sep)
  {
    // dir + "/" + file
    char* tmp = __OBZ_str_concat(dir, "/");
    char* out = __OBZ_str_concat(tmp, file);
    obz_free(tmp);
    return out;
  }

  // dir already ends with separator → dir + file
  return __OBZ_str_concat(dir, file);
}

inline static char* __OBZ_skipws(char* s)
{
  while (*s && isspace((uchar)*s))
    s++;
  return s;
}

/* ------------------------------
   Custom atoi -> moves pointer past digits and optional sign
   returns parsed integer (does not skip leading whitespace)
   ------------------------------ */
inline static int __OBZ_atoi_ptr(const char** ptr)
{
  const char* p    = *ptr;
  int         sign = 1, v = 0;
  if (*p == '-')
  {
    sign = -1;
    p++;
  }
  while (*p >= '0' && *p <= '9')
  {
    v = v * 10 + (*p - '0');
    p++;
  }
  *ptr = p;
  return v * sign;
}

OBZ_END_CPP_DECLS

#endif
