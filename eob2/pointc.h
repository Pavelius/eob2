#pragma once

const int mpx = 38;
const int mpy = 23;

struct pointc {
	char     x = -1, y = -1;
	constexpr bool operator==(pointc v) const { return x == v.x && y == v.y; }
	constexpr bool operator!=(pointc v) const { return x != v.x || y != v.y; }
	explicit operator bool() const { return x >= 0 && y >= 0 && x < mpx && y < mpy; }
	pointc operator+(const pointc& v) const { return {x + v.x, y + v.y}; }
	pointc operator+(int i) const { pointc v; v.set(x + i, y + i); return v; }
	pointc operator-(int i) const { pointc v; v.set(x - i, y - i); return v; }
	void	clear() { x = y = -1; }
	int		distance(pointc v) const;
	pointc	to(int dx, int dy) const { return {(char)(x + dx), (char)(y + dy)}; }
	void set(int x1, int y1) {
		if(x1 < 0) x1 = 0; if(x1 >= mpx) x1 = mpx - 1;
		if(y1 < 0) y1 = 0; if(y1 >= mpy) x1 = mpy - 1;
		x = (char)x1;
		y = (char)y1;
	}
};
extern pointc last_point;

typedef bool (*fnpointc)(pointc v);
