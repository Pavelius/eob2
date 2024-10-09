#pragma once

#include "nameable.h"

enum abilityn : unsigned char {
	Strenght, Dexterity, Constitution, Intellegence, Wisdow, Charisma,
	SaveVsParalization, SaveVsPoison, SaveVsTraps, SaveVsMagic,
	ClimbWalls, HearNoise, MoveSilently, OpenLocks, RemoveTraps, ReadLanguages,
	LearnSpell,
	ResistMagic,
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
	int minimal = 0;
	int maximal = 100;
};
void add_value(char& result, int i, int minimum = 0, int maximum = 120);

extern abilityn last_ability;