#pragma once

#include "dice.h"
#include "nameable.h"

struct randomeffecti : nameable {
	dice base, raise;
	int	perlevel[3], multiplier;
	int	roll(int level) const;
};
extern randomeffecti* last_effect;
extern int last_level;
