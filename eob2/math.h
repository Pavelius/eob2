/////////////////////////////////////////////////////////////////////////
// 
// Copyright 2024 Pavel Chistyakov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http ://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

template<class T> inline T iabs(T a) { return a > 0 ? a : -a; }
template<class T> inline T imax(T a, T b) { return a > b ? a : b; }
template<class T> inline T imin(T a, T b) { return a < b ? a : b; }
template<class T> inline void iswap(T& a, T& b) { T i = a; a = b; b = i; }

int isqrt(int num);
float sqrt(const float x);

#define maptbl(t, v) (t[imax((int)0, imin((int)v, (int)(sizeof(t)/sizeof(t[0])-1)))])
#define FG(v) (1<<v)