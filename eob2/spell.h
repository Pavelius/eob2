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

struct spelli : nameable, featable  {
	char		levels[4];
	color		lighting;
	char		avatar_thrown;
	const randomeffecti* duration; // Enchantment spell
	const itemi* summon; // Which item summoned in hand
	variants	filter, filter_item;
	variants	instant, wearing, clearing;
	flag32		filter_cell;
	bool		isthrown() const { return avatar_thrown != 0; }
	int			getindex() const;
};
extern const spelli* last_spell;

typedef char spella[256];
typedef flagable<8, unsigned> spellseta;

void apply_enchant_spell(int bonus);
void camp_autocast();
bool can_cast_spell(int type, int level);
bool can_learn_spell(int type, int level);
void cast_spell();
void cast_spell(int bonus);
bool cast_spell(const spelli* ps, int level, int experience, bool run, bool random_target, unsigned durations, creaturei* explicit_target);
void clear_spellbook();

spellseta* get_spells_known(const creaturei* target);