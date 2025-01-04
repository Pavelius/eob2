#pragma once

#include "pointc.h"

enum directions : unsigned char;

struct shapei {
	const char*		id;
	const char*		content;
	pointc			origin;
	pointc			size;
	pointc			points[10];
	char operator[](pointc m) const { return content[m.y * size.x + m.x]; }
	pointc			center(pointc c) const { return c + origin; }
	void			clear();
	pointc			find(char sym) const;
	size_t			maximum() const { return size.x * size.y; }
	pointc			translate(pointc s, pointc m, directions d) const;
};
void shape_read(const char* url);
