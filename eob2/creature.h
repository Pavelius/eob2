#pragma once

#include "statable.h"
#include "levelable.h"
#include "spell.h"
#include "npc.h"
#include "wearable.h"

struct classi;
struct racei;

struct creaturei : npc, statable, levelable, wearable {
	statable		basic;
	short			hp, hpm, hpr;
	unsigned char	avatar;
	spella			spells;
	spellseta		knownspells;
	void			additem(item& it);
	void			clear();
	int				get(abilityn v) const { return abilities[v]; }
	const char*		getbadstate() const;
	int				getchance(abilityn v) const;
	dice			getdamage(wearn id) const;
	bool			is(abilityn v) const { return abilities[v] > 0; }
	bool			is(featn v) const { return featable::is(v); }
	bool			isallow(const item& it) const;
	bool			isdead() const { return hp <= -10; }
	bool			isdisabled() const { return hp <= 0; }
	bool			roll(abilityn v, int bonus = 0) const;
};
extern creaturei* player;

void add_spells(int type, int level, const spellseta* include);
void create_player(const racei* pr, gendern gender, const classi* pc);
void update_player();

creaturei* item_owner(const void* p);
wearn item_wear(const void* p);
bool can_remove(const item* pi, bool speech = true);