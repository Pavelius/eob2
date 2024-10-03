#pragma once

#include "dice.h"
#include "feat.h"
#include "nameable.h"
#include "variant.h"

struct itemi;

struct spelli : nameable, featable {
	char		levels[3];
	dice		effect, effect_add;
	char		effect_progress[3]; // start, per level, stop
	const itemi* summon; // Which item summoned in hand
	variants	wearing;
};