#pragma once

#include "nameable.h"

enum featn : unsigned char {
	NoExeptionalStrenght,
	BonusSaveVsPoison, BonusSaveVsSpells,
	BonusVsElfWeapon, BonusAttackVsGoblinoid, BonusDamageVsEnemy, BonusACVsLargeEnemy, BonusHP,
	WearMetal, WearLeather, WearMartial, WearShield, WearElf, WearRogue,
	Ambidextrity, Undead,
	ResistBludgeon, ResistSlashing, ResistPierce, ResistFire, ResistCold, ResistCharm, ResistSpells,
	ImmuneNormalWeapon, ImmuneFire, ImmuneCold, ImmuneCharm, ImmuneSpells, ImmuneDisease,
};
struct feati : nameable {
};
struct featable {
	unsigned feats;
	bool is(featn v) const { return (feats & (1 << v)) != 0; }
};
