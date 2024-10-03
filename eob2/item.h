#pragma once

#include "damage.h"
#include "dice.h"
#include "feat.h"
#include "nameable.h"
#include "variant.h"

struct itemi : nameable, featable {
	damagen harm;
	char attack, number_attacks;
	dice damage, damage_large;
	itemi* ammo;
	variants wearing;
};
class item {
	unsigned char type;
	union {
		struct {
			unsigned char cursed : 1;
			unsigned char identified : 1;
			unsigned char rserved : 6;
		};
		unsigned char flags;
	};
	unsigned char power;
	unsigned char count; // this one can be broken, charges, count.
};