#pragma once

#include "nameable.h"

//enum racen : unsigned char {
//	Dwarf, Elf, HalfElf, Halfling, Human,
//	Humanoid, Goblinoid, Insectoid, Animal,
//};
typedef char abilitya[6];
struct racei : nameable {
	abilitya	minimal, maximal;
};
