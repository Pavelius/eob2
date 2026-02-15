#pragma once

typedef void(*fnevent)();

struct keybindi {
	unsigned	key;
	fnevent		proc;
	int			type; // Numeric type of keyboard group
	const char* shortcut; // Named key need to load
};

void initialize_keybind();
