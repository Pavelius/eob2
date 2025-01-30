#pragma once

#include "flagable.h"
#include "nameable.h"

typedef char abilitya[6];
enum racen : unsigned char;

typedef flagable<1, unsigned> racef;

struct racei : nameable {
	abilitya	minimal, maximal;
	abilityn	ability;
	racen		origin;
	flag32		languages;
	flag32		specialization;
};
extern racen last_race;