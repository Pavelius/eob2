#include "math.h"
#include "pointc.h"

pointc last_point;

int pointc::distance(pointc v) const {
	auto dx = x - v.x;
	auto dy = y - v.y;
	return isqrt(dx * dx + dy * dy);
}

void pointc::set(int x1, int y1) {
	if(x1 < 0)
		x1 = 0;
	else if(x1 >= mpx)
		x1 = mpx - 1;
	if(y1 < 0)
		y1 = 0;
	if(y1 >= mpy)
		x1 = mpy - 1;
	x = (char)x1;
	y = (char)y1;
}