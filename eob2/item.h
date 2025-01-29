#pragma once

#include "dice.h"
#include "feat.h"
#include "nameable.h"
#include "purpose.h"
#include "variant.h"

struct listi;

enum wearn : unsigned char;
enum damagen : unsigned char;
struct itemi : nameable, featable {
	char		attack, number_attacks, speed;
	damagen		damage_type;
	dice		damage, damage_large;
	itemi*		ammo;
	variants	wearing;
	variant		cursed;
	wearn		wear;
	char		avatar, avatar_ground, avatar_thrown, chance_identify;
	int			cost;
	listi*		powers;
};
class item {
	unsigned char type;
	union {
		struct {
			unsigned char cursed : 1;
			unsigned char identified : 1;
			purposen purpose : 3;
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
	void		createpower(char magic_bonus, int chance_magical, int chance_cursed);
	void		curse(int v) { cursed = (v >= 0) ? 1 : 0; }
	void		damage(int v);
	const itemi& geti() const;
	void		identify(int v) { identified = (v >= 0) ? 1 : 0; }
	bool		is(featn v) const { return geti().is(v); }
	bool		is(const itemi* pi) const { return pi == &geti(); }
	bool		is(wearn v) const { return geti().wear == v; }
	bool		is(purposen v) const { return purpose == v; }
	bool		isallow(wearn v) const;
	bool		isartifact() const { return getpower().counter >= 4; }
	bool		iscountable() const;
	bool		iscursed() const { return cursed != 0; }
	bool		isdamaged() const { return !iscountable() && count >= 5; }
	bool		isidentified() const { return identified != 0; }
	bool		ismagical() const { return iscursed() || getpower().counter != 0; }
	bool		isranged() const { return geti().avatar_thrown || geti().ammo != 0; }
	bool		isweapon() const;
	bool		join(item& it);
	int			getcost() const;
	int			getcount() const { return iscountable() ? count + 1 : 1; }
	const char*	getname() const;
	void		getname(stringbuilder& sb) const;
	int			getmagic() const;
	variant		getpower() const;
	void		set(purposen v) { purpose = v; }
	void		setcount(int v);
	void		setpower(variant v);
	void		consume() { setcount(getcount() - 1); }
	void		usecharge(const char* interactive, int chance = 35, int use = 1); // Maximum charges is always 10
};
extern item* last_item;
