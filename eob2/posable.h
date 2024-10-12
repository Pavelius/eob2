#pragma once

#include "pointc.h"

enum directions : unsigned char;

struct posable : pointc {
	char		side = 0;
	directions	d = (directions)0;
	posable() = default;
	posable(pointc v) : pointc(v), side(0), d() {}
	posable(pointc v, directions d) : pointc(v), side(0), d(d) {}
	void		clear() { pointc::clear(); side = 0; d = (directions)0; }
	void		set(pointc v, directions d) { x = v.x; y = v.y; d = d; }
};
pointc to(pointc v, directions d);