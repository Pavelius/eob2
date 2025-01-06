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
	char	levels[4];
	char	avatar_thrown;
	color	lighting;
	const randomeffecti* duration; // Enchantment spell
	const itemi* summon; // Which item summoned in hand
	variants filter, filter_item;
	variants instant;
	variants clearing;
	variants wearing;
	flag32 filter_cell;
	bool isthrown() const { return avatar_thrown!=0; }
};
extern const spelli* last_spell;

typedef char spella[256];
typedef flagable<8, unsigned> spellseta;

void apply_enchant_spell(int bonus);
void cast_spell();
bool cast_spell(const spelli* ps, int level, int experience, bool run, bool random_target, unsigned durations, creaturei* explicit_target);

spellseta* get_spells_known(const creaturei* target);
