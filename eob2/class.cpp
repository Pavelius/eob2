#include "bsdata.h"
#include "class.h"

BSDATA(classi) = {
	{"NoClass"},
	{"Cleric"},
	{"Fighter"},
	{"Mage"},
	{"Paladin"},
	{"Ranger"},
	{"Theif"},
	{"FighterCleric"},
	{"FighterMage"},
	{"FighterTheif"},
	{"FighterMageTheif"},
	{"ClericTheif"},
	{"MageTheif"},
};
assert_enum(classi, MageTheif)