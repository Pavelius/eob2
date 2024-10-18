#pragma once

#include "dice.h"
#include "feat.h"
#include "nameable.h"
#include "variant.h"

enum wearn : unsigned char;

struct itemi : nameable, featable {
	char		attack, number_attacks, speed;
	dice		damage, damage_large;
	itemi*		ammo;
	variants	wearing, use;
	wearn		wear;
	char		avatar, avatar_ground, avatar_thrown;
	int			cost;
};
class item {
	unsigned char type;
	union {
		struct {
			unsigned char cursed : 1;
			unsigned char identified : 1;
			unsigned char reserved : 6;
		};
		unsigned char flags;
	};
	unsigned char power;
	unsigned char count; // this one can be broken, charges, count.
public:
	explicit operator bool() const { return type != 0; }
	void		clear();
	void		create(int value);
	void		create(const itemi* pi);
	const itemi& geti() const;
	void		identify(int v) { identified = (v >= 0) ? 1 : 0; }
	bool		isallow(wearn v) const;
	bool		iscursed() const { return cursed != 0; }
	bool		isidentified() const { return identified != 0; }
	bool		ismagical() const { return false; }
	bool		isweapon() const;
	int			getcost() const;
	const char*	getname() const;
	void		curse(int v) { cursed = (v >=0) ? 1 : 0; }
};
extern item* last_item;
