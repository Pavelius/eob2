#include "math.h"
#include "pointc.h"

int pointc::distance(pointc v) const {
	auto dx = x - v.x;
	auto dy = y - v.y;
	return isqrt(dx * dx + dy * dy);
}