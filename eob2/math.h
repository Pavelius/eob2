#pragma once

template<class T> inline T iabs(T a) { return a > 0 ? a : -a; }
template<class T> inline T imax(T a, T b) { return a > b ? a : b; }
template<class T> inline T imin(T a, T b) { return a < b ? a : b; }
template<class T> inline void iswap(T& a, T& b) { T i = a; a = b; b = i; }

int isqrt(int num);
float sqrt(const float x);

#define maptbl(t, v) (t[imax((size_t)0, imin((size_t)v, sizeof(t)/sizeof(t[0])-1))])

