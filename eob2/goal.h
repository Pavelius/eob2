#pragma once

#include "nameable.h"
#include "flagable.h"

enum goaln : unsigned char {
	ExmploreMostArea,
	FindAllSecrets, TakeSpecialItem, OpenAllLockedDoors, DisableAllTraps,
	KillBoss, KillBossMinions, KillAlmostAllMonsters,
};
struct goali : nameable {
	typedef bool(*fntest)();
	fntest	test;
	int		experience;
};
typedef flag16 goalf;
typedef char goala[KillAlmostAllMonsters+1];
