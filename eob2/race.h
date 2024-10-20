#pragma once

#include "flagable.h"
#include "nameable.h"

typedef char abilitya[6];
enum racen : unsigned char;

struct racei : nameable {
	abilitya minimal, maximal;
};
extern racen last_race;

typedef flagable<1, unsigned> racef;