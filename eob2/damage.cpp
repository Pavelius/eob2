#include "bsdata.h"
#include "damage.h"
#include "feat.h"

BSDATA(damagei) = {
	{"Bludgeon", ResistBludgeon, ImmuneNormalWeapon},
	{"Slashing", ResistSlashing, ImmuneNormalWeapon},
	{"Piercing", ResistPierce, ImmuneNormalWeapon},
	{"Magic"},
	{"Fire", ResistFire, ImmuneFire},
	{"Cold", ResistCold, ImmuneCold},
	{"Acid"},
	{"Shock"},
	{"Poison"},
};
assert_enum(damagei, Poison)
