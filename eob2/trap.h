#pragma once

#include "damage.h"
#include "dice.h"
#include "nameable.h"

struct trapi : nameable {
	damagen	type;
	dice	damage;
	char	avatar_thrown;
	char	group; // 1 - if whole group targets
	char	save; // 0 - half if success, 1 - negate if success, -1 - none
};
