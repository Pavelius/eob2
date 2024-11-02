#include "direction.h"
#include "pointca.h"

pointca points;

void pointca::select(int r1, int r2) {
	points.clear();
	pointc v;
	for(v.x = 0; v.x < mpx; v.x++) {
		for(v.y = 0; v.y < mpy; v.y++) {
			if(pathmap[v.y][v.x] >= r1 && pathmap[v.y][v.x] <= r2)
				points.add(v);
		}
	}
}