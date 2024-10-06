#pragma once

typedef void(*fnevent)();

struct hotkeyi {
	unsigned	key;
	fnevent		proc;
	explicit operator bool() const { return key != 0; }
};