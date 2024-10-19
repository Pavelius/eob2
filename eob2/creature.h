#pragma once

#include "damage.h"
#include "statable.h"
#include "levelable.h"
#include "posable.h"
#include "race.h"
#include "spell.h"
#include "npc.h"
#include "wearable.h"

struct classi;
struct monsteri;

struct creaturei : npc, statable, levelable, wearable, posable {
	statable		basic;
	short			hp, hpm, hpr;
	short unsigned	monster_id;
	unsigned char	avatar;
	char			initiative;
	spella			spells;
	spellseta		knownspells;
	racef			hate;
	void			addexp(int value);
	void			additem(item& it);
	void			attack(creaturei* enemy, wearn slot, int bonus, int multiplier);
	void			clear();
	void			damage(damagen type, int hits, char magic_bonus = 0);
	int				get(abilityn v) const { return abilities[v]; }
	const char*		getbadstate() const;
	int				getchance(abilityn v) const;
	dice			getdamage(wearn id, bool large_enemy) const;
	int				getexpaward() const;
	int				gethitpenalty(int bonus) const;
	const monsteri*	getmonster() const;
	bool			is(abilityn v) const { return abilities[v] > 0; }
	bool			is(featn v) const { return featable::is(v); }
	bool			isactable() const;
	bool			isallow(const item& it) const;
	bool			isdead() const { return hp <= -10; }
	bool			isdisabled() const { return hp <= 0; }
	bool			ismonster() const { return getmonster() != 0; }
	void			kill();
	bool			roll(abilityn v, int bonus = 0) const;
	void			set(featn v) { featable::set(v); }
	void			setframe(short* frames, short index) const;
};
extern creaturei *player, *opponent;
extern int last_roll, last_chance;

void add_spells(int type, int level, const spellseta* include);
void create_player(const racei* pr, gendern gender, const classi* pc);
void create_monster(const monsteri* pi);
void update_player();

creaturei* item_owner(const void* p);
wearn item_wear(const void* p);
bool can_remove(const item* pi, bool speech = true);