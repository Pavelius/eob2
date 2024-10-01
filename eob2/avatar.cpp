#include "cflags.h"
#include "class.h"
#include "gender.h"
#include "race.h"
#include "rand.h"

static struct portraiti {
	gendern			gender;
	cflags<racen>	races;
	cflags<classn>	classes;
} portrait_data[] = {
	{Male, {Human, Halfling}, {Fighter, Ranger}},
	{Male, {Human}, {Mage, Cleric}},
	{Male, {Human}, {Mage, Cleric, Ranger}},
	{Male, {Human, Halfling}, {Theif}},
	{Male, {Elf}, {Theif, Mage, Fighter, Ranger}},
	{Male, {Elf, HalfElf}, {Theif, Fighter, Ranger}},
	{Male, {Dwarf}, {Cleric, Fighter}},
	{Male, {Human}, {Theif, Mage, Ranger}},
	{Male, {Human, Halfling}, {Theif, Fighter, Ranger, Cleric}},
	{NoGender, {Halfling}, {Theif, Fighter}},
	{Male, {Elf, HalfElf}, {Mage}},
	{Male, {Human}, {Theif}},
	{Male, {Human}, {Mage, Cleric}},
	{Male, {Elf, Human}, {Fighter, Paladin, Cleric}},
	{Male, {HalfElf, Human}, {Mage}},
	{Male, {Human}, {Fighter, Paladin}},
	{Male, {Elf, HalfElf}, {Mage, Ranger}},
	{Male, {Human}, {Fighter, Paladin}},
	{Male, {Human}, {Fighter, Paladin, Cleric}},
	{Male, {Human}, {Fighter, Paladin}},
	{Male, {Elf, HalfElf}, {Fighter, Paladin}},
	{Male, {Dwarf}, {Fighter}},
	{Male, {Elf, HalfElf}, {Fighter, Paladin, Ranger}},
	{Male, {Human}, {Fighter}},
	{Male, {Dwarf}, {Fighter}},
	{Male, {Human}, {Fighter}},
	{NoGender, {Human, Elf, HalfElf}, {Mage}},
	{NoGender, {Human}, {Mage}},
	{NoGender, {Human, Dwarf}, {Cleric}},
	{Female, {Elf}, {Mage, Cleric}},
	{Female, {Human}, {Theif, Ranger, Fighter}},
	{Female, {Human}, {Fighter, Cleric}},
	{Female, {Human}, {Fighter, Cleric}},
	{Female, {Human, Elf}, {Cleric, Mage}},
	{Female, {Elf, HalfElf}, {Cleric, Mage, Ranger}},
	{Female, {Human, HalfElf}, {Fighter, Paladin, Ranger}},
	{Female, {Human}, {Fighter, Cleric, Mage}},
	{Female, {Dwarf, Human}, {Cleric, Mage}},
	{Female, {Dwarf, Human}, {Cleric, Mage}},
	{Female, {Dwarf, Human}, {Fighter, Cleric, Mage}},
	{Female, {Human}, {Fighter, Cleric, Mage}},
	{Female, {Human, HalfElf}, {Cleric, Mage}},
	{Female, {Human, HalfElf, Elf}, {Cleric, Mage, Paladin}},
	{Female, {Human, HalfElf, Elf}, {Cleric, Mage, Paladin, Theif}},
	{Female, {Human}, {Fighter}},
	{Male, {Human}, {Fighter, Paladin}},
	{Female, {Human, HalfElf, Elf}, {Cleric, Mage}},
	{Female, {Human}, {Cleric, Mage}},
	{Female, {Human}, {Cleric, Mage, Theif}},
	{NoGender, {Halfling}, {Theif, Fighter}},
	{Male, {Dwarf}, {Fighter}},
	{Male, {Dwarf}, {Fighter, Cleric}},
	{Male, {Dwarf}, {Fighter, Cleric, Theif}},
	{Male, {Halfling}, {Fighter, Cleric, Theif}},
	{Female, {Human}, {Fighter, Cleric, Ranger}},
	{Male, {Dwarf}, {Cleric}},
	{Male, {Human, Elf, HalfElf}, {Mage}},
	{Male, {Dwarf}, {Fighter, Cleric}},
	{Female, {Human}, {Cleric, Mage}},
};

static size_t get_avatars_ex(unsigned char* result, racen race, gendern gender, classn cls) {
	auto p = result;
	for(auto& e : portrait_data) {
		if(e.gender != NoGender && e.gender != gender)
			continue;
		if(!e.races.is(race))
			continue;
		if(!e.classes.is(cls))
			continue;
		*p++ = &e - portrait_data;
	}
	return p - result;
}

static size_t get_avatars_ex(unsigned char* result, racen race, gendern gender) {
	auto p = result;
	for(auto& e : portrait_data) {
		if(e.gender != NoGender && e.gender != gender)
			continue;
		if(!e.races.is(race))
			continue;
		*p++ = &e - portrait_data;
	}
	return p - result;
}

size_t get_avatars(unsigned char* result, racen race, gendern gender, classn cls) {
	auto c = get_avatars_ex(result, race, gender, cls);
	if(c < 4)
		c = get_avatars_ex(result, race, gender);
	return c;
}

unsigned char get_avatar(racen race, gendern gender, classn cls) {
	unsigned char result[256];
	auto c = get_avatars(result, race, gender, cls);
	if(!c)
		return 0;
	return result[rand() % c];
}