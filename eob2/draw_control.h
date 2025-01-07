#pragma once

typedef void(*fnevent)();
enum resid : unsigned short;

struct controli {
	fnevent		proc;
	short		x, y;
	int			width, height;
	resid		res;
	int			params[4];
	const char*	id;
};
extern const controli* last_control;

void paint_controls(const controli* controls, size_t count);