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

#include "math.h"
#include "point.h"

bool point::in(const point p1, const point p2, const point p3) const {
	int a = (p1.x - x) * (p2.y - p1.y) - (p2.x - p1.x) * (p1.y - y);
	int b = (p2.x - x) * (p3.y - p2.y) - (p3.x - p2.x) * (p2.y - y);
	int c = (p3.x - x) * (p1.y - p3.y) - (p1.x - p3.x) * (p3.y - y);
	return (a >= 0 && b >= 0 && c >= 0)
		|| (a < 0 && b < 0 && c < 0);
}

long distance(point p1, point p2) {
	auto dx = p1.x - p2.x;
	auto dy = p1.y - p2.y;
	return isqrt(dx * dx + dy * dy);
}