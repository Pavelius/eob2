#pragma once

#include "nameable.h"

struct talking : nameable {
	typedef bool(*papply)(bool run);
	papply proc;
};
extern talking* last_talk;
