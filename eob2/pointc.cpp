#include "math.h"
#include "pointc.h"

pointc last_point;

int pointc::distance(pointc v) const {
	auto dx = x - v.x;
	auto dy = y - v.y;
	return isqrt(dx * dx + dy * dy);
}