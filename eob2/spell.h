#pragma once

#include "color.h"
#include "dice.h"
#include "feat.h"
#include "flagable.h"
#include "nameable.h"
#include "randomeffect.h"
#include "variant.h"

struct itemi;

struct spelli : nameable, featable {
	char		levels[4];
	char		thrown;
	color		lighting;
	const randomeffecti* effect; // Damage or other effect
	const randomeffecti* duration; // Enchantment spell
	const itemi* summon; // Which item summoned in hand
	variants	filter, instant, wearing;
};
typedef char spella[128];
typedef flagable<16> spellseta;

void cast_spell();