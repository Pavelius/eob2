#pragma once

struct commandi;
typedef void(*fnevent)();

struct keybindi {
	unsigned	key;
	const char* id_key; // Named key need to load
	commandi*	command;
	int			type; // Numeric type of keyboard group
};

void initialize_keybind();
