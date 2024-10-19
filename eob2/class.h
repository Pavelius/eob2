#pragma once

#include "adat.h"
#include "feat.h"
#include "nameable.h"

enum abilityn : unsigned char;
typedef char abilitya[6];

struct classi : nameable {
	char	classes[3], count, hd;
	int		exp_per_hd; // multiplied by 100
	abilitya minimal;
	abilityn primary;
};
extern classi* last_class;