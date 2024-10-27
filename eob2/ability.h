#pragma once

#include "nameable.h"

enum abilityn : unsigned char {
	Strenght, Dexterity, Constitution, Intellegence, Wisdow, Charisma,
	SaveVsParalization, SaveVsPoison, SaveVsTraps, SaveVsMagic,
	ClimbWalls, HearNoise, MoveSilently, OpenLocks, PickPockets, RemoveTraps, ReadLanguages,
	LearnSpell,
	ResistMagic, Sneaky, Alertness,
	CriticalDeflect, DetectSecrets,
	AC,
	AttackMelee, AttackRange,
	DamageMelee, DamageRange,
	Speed,
	Spell1, Spell2, Spell3, Spell4, Spell5, Spell6, Spell7, Spell8, Spell9, Spells,
	BonusExperience, ReactionBonus,
	ExeptionalStrenght,
	PoisonLevel, DiseaseLevel,
	DrainStrenght, DrainConstitution, DrainLevel,
	Hits
};
struct abilityi : nameable {
};
extern abilityn last_ability;