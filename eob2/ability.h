#pragma once

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
	BonusExperience, ReactionBonus,
	ExeptionalStrenght,
	Hits
};
struct abilityi {
	const char* id;
	int minimal = 0;
	int maximal = 100;
};
void add_value(char& result, int i, int minimum = 0, int maximum = 120);