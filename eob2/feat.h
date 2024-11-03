#pragma once

#include "nameable.h"

enum featn : unsigned char {
	NoExeptionalStrenght,
	BonusSaveVsPoison, BonusSaveVsSpells,
	BonusVsElfWeapon, BonusAttackVsHated, BonusDamageVsEnemy, BonusACVsLargeEnemy, BonusHP,
	UseMetal, UseLeather, UseShield,
	UseMartial, UseElvish, UseRogish, UsePriest, UseMage, TwoHanded, Precise, Deadly, Unique,
	DiseaseAttack, DrainStrenghtAttack, DrainEneryAttack, ParalizeAttack, VampiricAttack, WeaponSpecialist,
	Small, Large, Undead, Paralized, Moved, Surprised, ProtectedFromEvil, Invisibled, Regenerated, Hasted, SlowMove, Displaced,
	ResistBludgeon, ResistSlashing, ResistPierce, ResistFire, ResistCold, ResistCharm, ResistSpells,
	ImmuneNormalWeapon, ImmuneFire, ImmuneCold, ImmuneCharm, ImmunePoison, ImmuneSpells, ImmuneDisease,
	Enemy, Ally, Group, You, WearItem,
	SeeMagical, SeeCursed, SeeIllusionary,
};
struct feati : nameable {
};
struct featable {
	unsigned feats[2];
	bool is(featn v) const { return (feats[v / 32] & (1 << (v % 32))) != 0; }
	void remove(featn v) { feats[v / 32] &= ~(1 << (v % 32)); }
	void set(featn v) { feats[v / 32] |= (1 << (v % 32)); }
};
