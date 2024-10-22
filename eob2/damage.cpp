#include "bsdata.h"
#include "damage.h"
#include "feat.h"

BSDATA(damagei) = {
	{"Bludgeon", ResistBludgeon, ImmuneNormalWeapon},
	{"Slashing", ResistSlashing, ImmuneNormalWeapon},
	{"Piercing", ResistPierce, ImmuneNormalWeapon},
	{"Fire", ResistFire},
	{"Cold", ResistCold},
	{"Acid"},
	{"Shock"},	
};
assert_enum(damagei, Shock)
