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

#include "rect.h"

#pragma once

struct point {
	short int	x, y;
	constexpr explicit operator bool() const { return x || y; }
	bool			operator!=(const point pt) const { return pt.x != x || pt.y != y; }
	bool			operator==(const point pt) const { return pt.x == x && pt.y == y; }
	point			operator-(const point pt) const { return{(short)(x - pt.x), (short)(y - pt.y)}; }
	point			operator+(const point pt) const { return{(short)(x + pt.x), (short)(y + pt.y)}; }
	void			clear() { x = y = 0; }
	bool			in(const rect& rc) const { return x >= rc.x1 && x <= rc.x2 && y >= rc.y1 && y <= rc.y2; }
	bool			in(const point p1, const point p2, const point p3) const;
	static point create(int n) { return {(short)((unsigned)n & 0xFFFF), (short)(((unsigned)n) >> 16)}; }
	void			set(int px, int py) { x = (short)px; y = (short)py; }
};
long distance(point p1, point p2);
