#pragma once

enum featn : unsigned char {
	AllowExeptionalStrenght,
	BonusSaveVsPoison, BonusSaveVsSpells,
	BonusVsElfWeapon, BonusAttackVsGoblinoid, BonusDamageVsEnemy, BonusACVsLargeEnemy, BonusHP,
	Ambidextrity, Undead,
	ResistBludgeon, ResistSlashing, ResistPierce, ResistFire, ResistCold, ResistCharm, ResistSpells,
	ImmuneNormalWeapon, ImmuneFire, ImmuneCold, ImmuneCharm, ImmuneSpells, ImmuneDisease,
};
struct feati {
	const char* id;
};
struct featable {
	unsigned feats;
	bool is(featn v) const { return (feats & (1 << v)) != 0; }
};
