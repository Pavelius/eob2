#pragma once

#include "dice.h"
#include "feat.h"
#include "nameable.h"
#include "variant.h"

enum wearn : unsigned char;
enum damagen : unsigned char;

struct itemi : nameable, featable {
	char		attack, number_attacks, speed;
	damagen		damage_type;
	dice		damage, damage_large;
	itemi*		ammo;
	variants	wearing, use, onhit;
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
			unsigned char summoned : 1;
			unsigned char reserved : 5;
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
	void		curse(int v) { cursed = (v >= 0) ? 1 : 0; }
	void		damage(int v);
	const itemi& geti() const;
	void		identify(int v) { identified = (v >= 0) ? 1 : 0; }
	bool		is(featn v) const { return geti().is(v); }
	bool		is(const itemi* pi) const { return pi == &geti(); }
	bool		isallow(wearn v) const;
	bool		iscursed() const { return cursed != 0; }
	bool		isdamaged() const { return count >= 5; }
	bool		isidentified() const { return identified != 0; }
	bool		isnatural() const { return is(You); }
	bool		ismagical() const { return iscursed() || getpower(); }
	bool		isranged() const { return false; }
	bool		issummoned() const { return summoned != 0; }
	bool		isweapon() const;
	int			getcost() const;
	const char*	getname() const;
	variant		getpower() const { return variant(); }
	void		summon(int v) { summoned = (v >= 0) ? 1 : 0; }
};
extern item* last_item;
