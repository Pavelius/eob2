#pragma once

#include "adat.h"
#include "feat.h"
#include "nameable.h"

enum classn : unsigned char {
	NoClass,
	Cleric, Fighter, Mage, Paladin, Ranger, Theif,
	FighterCleric, FighterMage, FighterTheif, FighterMageTheif,
	ClericTheif, MageTheif,
};
struct classi : nameable, featable {
	adat<classn, 3> classes;
};