#pragma once

#include "adat.h"
#include "alignment.h"
#include "feat.h"
#include "nameable.h"

enum abilityn : unsigned char;
enum classn : unsigned char;

typedef char abilitya[6];

struct classi : nameable, featable {
	classn		classes[3];
	char		count, hd, caster, experience;
	char		save_group; // Calculation saving throws: 0 warrior, 1 priest, 2 rogues, 3 wizards
	int			exp_per_hd; // multiplied by 100
	abilitya	minimal;
	abilityn	primary;
	alignmenta	alignment; // Allowed alignments or zero if allowed all.
	flag32		races; // Allowed races or zero if allowed all.
	char		non_player;
};
extern classn last_class;