#pragma once

#include "color.h"
#include "dice.h"
#include "feat.h"
#include "flagable.h"
#include "nameable.h"
#include "randomeffect.h"
#include "variant.h"

struct itemi;
struct creaturei;

struct spelli : nameable, featable {
	char		levels[4];
	char		avatar_thrown;
	color		lighting;
	const randomeffecti* duration; // Enchantment spell
	const itemi* summon; // Which item summoned in hand
	variants	filter;
	variants	instant;
	variants	clearing;
	variants	wearing;
	bool isthrown() const { return avatar_thrown!=0; }
};
extern spelli* last_spell;
typedef char spella[128];
typedef flagable<16> spellseta;

void cast_spell();
bool cast_spell(const spelli* ps, int level, int experience, bool run);

spellseta* get_spells_known(const creaturei* target);
