#pragma once

enum directions : unsigned char;

const int mpx = 38;
const int mpy = 23;

struct pointc {
	char		x = -1, y = -1;
	constexpr bool operator==(pointc v) const { return x==v.x && y==v.y; }
	explicit operator bool() const { return x >= 0 && y >= 0 && x < mpx && y < mpy; }
	pointc operator+(const pointc& v) const { return {x + v.x, y + v.y}; }
	void		clear() { x = y = -1; }
};
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