#include "bsdata.h"
#include "feat.h"

BSDATA(feati) = {
	{"NoExeptionalStrenght"},
	{"BonusSaveVsPoison"}, {"BonusSaveVsSpells"},
	{"BonusVsElfWeapon"}, {"BonusAttackVsGoblinoid"}, {"BonusDamageVsEnemy"}, {"BonusACVsLargeEnemy"}, {"BonusHP"},
	{"WearMetal"}, {"WearLeather"},
	{"Ambidextrity"}, {"Undead"},
	{"ResistBludgeon"}, {"ResistSlashing"}, {"ResistPierce"}, {"ResistFire"}, {"ResistCold"}, {"ResistCharm"}, {"ResistSpells"},
	{"ImmuneNormalWeapon"}, {"ImmuneFire"}, {"ImmuneCold"}, {"ImmuneCharm"}, {"ImmuneSpells"}, {"ImmuneDisease"},
};
assert_enum(feati, ImmuneDisease)