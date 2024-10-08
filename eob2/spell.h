#pragma once

#include "dice.h"
#include "feat.h"
#include "flagable.h"
#include "nameable.h"
#include "randomeffect.h"
#include "variant.h"

struct itemi;

struct spelli : nameable, featable {
	char levels[3];
	char thrown;
	const randomeffecti* effect; // Damage or other effect
	const randomeffecti* duration; // Enchantment spell
	const itemi* summon; // Which item summoned in hand
	variants wearing;
};
typedef char spella[128];
typedef flagable<16> spellseta;