#pragma once

#include "adat.h"
#include "alignment.h"
#include "feat.h"
#include "nameable.h"

enum abilityn : unsigned char;
enum classn : unsigned char {
	Monster,
	Fighter, Cleric, Mage, Theif, Ranger, Paladin,
	FighterCleric, FighterTheif, FighterMage, MageTheif,
	FighterMageTheif
};

typedef char abilitya[6];

struct classi : nameable {
	classn		classes[3];
	char		count, hd, caster;
	char		save_group; // Calculation saving throws: 0 warrior, 1 priest, 2 wizards, 3 rogues
	int			exp_per_hd; // multiplied by 100
	abilitya	minimal;
	abilityn	primary;
	alignmenta	alignment; // Allowed alignments or zero if allowed all.
};
extern classn last_class;