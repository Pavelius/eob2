#pragma once

//enum racen : unsigned char {
//	Dwarf, Elf, HalfElf, Halfling, Human,
//	Humanoid, Goblinoid, Insectoid, Animal,
//};
typedef char abilitya[6];
struct racei {
	const char*	id;
	abilitya	minimal, maximal;
};
