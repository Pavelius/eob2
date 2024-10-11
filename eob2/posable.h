#pragma once

enum directions : unsigned char;

struct pointc {
	char		x = -1, y = -1;
	explicit operator bool() const { return x < 0; }
	pointc operator+(const pointc& v) const { return {x + v.x, y + v.y}; }
	void		clear() { x = y = -1; }
};
struct posable : pointc {
	char		side = 0;
	directions	direction = (directions)0;
	void		clear() { pointc::clear(); side = 0; direction = (directions)0; }
};
