#pragma once

const int mpx = 38;
const int mpy = 23;

struct pointc {
	char     x = -1, y = -1;
	constexpr bool operator==(pointc v) const { return x == v.x && y == v.y; }
	constexpr bool operator!=(pointc v) const { return x != v.x || y != v.y; }
	explicit operator bool() const { return x >= 0 && y >= 0 && x < mpx && y < mpy; }
	pointc operator+(const pointc& v) const { return {x + v.x, y + v.y}; }
	void		clear() { x = y = -1; }
	int		distance(pointc v) const;
	pointc	to(int dx, int dy) const { return {(char)(x + dx), (char)(y + dy)}; }
};
