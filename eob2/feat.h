#pragma once

#include "nameable.h"

enum featn : unsigned char {
	NoExeptionalStrenght,
	BonusSaveVsPoison, BonusSaveVsSpells,
	BonusVsElfWeapon, BonusAttackVsHated, BonusDamageVsEnemy, BonusACVsLargeEnemy, BonusHP,
	UseMetal, UseLeather, UseShield,
	UseMartial, UseElvish, UseRogish, UsePriest, UseMage, TwoHanded, Precise, Deadly, Unique,
	DiseaseAttack, DrainStrenghtAttack, DrainEneryAttack, ParalizeAttack, PoisonAttack, VampiricAttack, VorpalAttack, Holy, WeaponSpecialist,
	Small, Large, Undead, Paralized, Moved, Surprised,
	ProtectedFromEvil, Invisibled, Regenerated, Hasted, StoppedPoison, SlowMove, FeelPain, Displaced,
	Blinded, Blinked, Blurred,
	ResistBludgeon, ResistSlashing, ResistPierce, ResistFire, ResistCold, ResistCharm, ResistSpells,
	ImmuneNormalWeapon, ImmuneFire, ImmuneCold, ImmuneCharm, ImmunePoison, ImmuneSpells, ImmuneDisease,
	Enemy, Ally, Group, You, SummaryEffect,
	SeeMagical, SeeCursed, ImmuneIllusion,
};
struct feati : nameable {
};
struct featable {
	unsigned feats[4];
	bool is(featn v) const { return (feats[v / 32] & (1 << (v % 32))) != 0; }
	void remove(featn v) { feats[v / 32] &= ~(1 << (v % 32)); }
	void set(featn v) { feats[v / 32] |= (1 << (v % 32)); }
	void set(featn v, bool apply) { if(apply) set(v); else remove(v); }
};
