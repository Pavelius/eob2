#pragma once

const int mpx = 38;
const int mpy = 23;

struct pointc {
	char     x = -1, y = -1;
	constexpr bool operator==(pointc v) const { return x == v.x && y == v.y; }
	constexpr bool operator!=(pointc v) const { return x != v.x || y != v.y; }
	explicit operator bool() const { return x >= 0 && y >= 0 && x < mpx && y < mpy; }
	pointc operator+(const pointc& v) const { return {(char)(x + v.x), (char)(y + v.y)}; }
	pointc operator+(int i) const { pointc v; v.set(x + i, y + i); return v; }
	pointc operator-(int i) const { pointc v; v.set(x - i, y - i); return v; }
	void	clear() { x = y = -1; }
	int		distance(pointc v) const;
	void	set(int x1, int y1);
	pointc	to(int dx, int dy) const { return {(char)(x + dx), (char)(y + dy)}; }
};
extern pointc last_point;

typedef bool (*fnpointc)(pointc v);
