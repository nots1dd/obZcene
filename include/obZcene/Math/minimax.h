#ifndef OBZ_FAST_MINMAX_H
#define OBZ_FAST_MINMAX_H

inline static int obz_mini(int a, int b) { return (a < b) ? a : b; }
inline static int obz_maxi(int a, int b) { return (a > b) ? a : b; }

inline static float obz_minf(float a, float b) { return (a < b) ? a : b; }
inline static float obz_maxf(float a, float b) { return (a > b) ? a : b; }

inline static double obz_mind(double a, double b) { return (a < b) ? a : b; }
inline static double obz_maxd(double a, double b) { return (a > b) ? a : b; }

#endif /* OBZ_FAST_MINMAX_H */
