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
	char		classes[3], count, hd, caster;
	int			exp_per_hd; // multiplied by 100
	abilitya	minimal;
	abilityn	primary;
	alignmenta	alignment; // Allowed alignments or zero if allowed all.
};
extern classn last_class;