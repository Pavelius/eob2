#pragma once

#include "damage.h"
#include "dice.h"
#include "feat.h"
#include "nameable.h"
#include "variant.h"

enum wearn : unsigned char;

struct itemi : nameable, featable {
	damagen		harm;
	char		attack, number_attacks, speed;
	dice		damage, damage_large;
	itemi*		ammo;
	variants	wearing, use;
	wearn		wear;
	char		avatar, avatar_ground;
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
public:
	explicit operator bool() const { return type != 0; }
	void			clear();
	void			create(int value);
	const itemi&	geti() const;
	bool			isallow(wearn v) const;
};
extern item* last_item;