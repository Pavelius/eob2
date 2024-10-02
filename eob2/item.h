#pragma once

#include "dice.h"
#include "feat.h"
#include "nameable.h"

struct itemi : nameable, featable {
	char attack, number_attacks;
	dice damage, damage_large;
};
