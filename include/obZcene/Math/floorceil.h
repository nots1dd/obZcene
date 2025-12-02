#ifndef OBZ_FAST_FLOORCEIL_H
#define OBZ_FAST_FLOORCEIL_H

#define MAGIC_NO_FLOAT_TRUNCATE  8388608.0f          // 2^23 (~8M)
#define MAGIC_NO_DOUBLE_TRUNCATE 4503599627370496.0f // 2^52 (~4.5e15)

inline static float obz_floorf(float x)
{
  float r = x + MAGIC_NO_FLOAT_TRUNCATE;
  r -= MAGIC_NO_FLOAT_TRUNCATE;
  return r > x ? r - 1.0f : r;
}

inline static float obz_ceilf(float x)
{
  const float magic = MAGIC_NO_FLOAT_TRUNCATE;
  float       r     = x + magic;
  r -= magic;
  return r < x ? r + 1.0f : r;
}

inline static double obz_floord(double x)
{
  const double magic = MAGIC_NO_DOUBLE_TRUNCATE;
  double       r     = x + magic;
  r -= magic;
  return r > x ? r - 1.0 : r;
}

inline static double obz_ceild(double x)
{
  const double magic = MAGIC_NO_DOUBLE_TRUNCATE;
  double       r     = x + magic;
  r -= magic;
  return r < x ? r + 1.0 : r;
}

#endif /* OBZ_FAST_FLOORCEIL_H */
