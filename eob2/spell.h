#pragma once

#include "dice.h"
#include "feat.h"
#include "nameable.h"
#include "randomeffect.h"
#include "variant.h"

struct itemi;

struct spelli : nameable, featable {
	char levels[3];
	char thrown;
	const randomeffecti* effect; // Damage or other effect
	const randomeffecti* duration; // Enchantment spell
	const itemi* summon; // Which item summoned in hand
	variants wearing;
};
struct spella {
	unsigned	spells[4];
	bool		is(int v) const { return (spells[v / 32] & (v << (v % 32))) != 0; }
	void		remove(int v) { (spells[v / 32] &= ~(v << (v % 32))); }
	void		set(int v) { (spells[v / 32] |= (v << (v % 32))); }
};