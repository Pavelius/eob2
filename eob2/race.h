#pragma once

#include "flagable.h"
#include "nameable.h"

typedef char abilitya[6];
struct racei : nameable {
	abilitya minimal, maximal;
};
extern racei* last_race;

typedef flagable<1, unsigned> racef;