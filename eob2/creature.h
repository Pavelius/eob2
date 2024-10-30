#pragma once

#include "class.h"
#include "damage.h"
#include "statable.h"
#include "levelable.h"
#include "posable.h"
#include "race.h"
#include "spell.h"
#include "npc.h"
#include "wearable.h"

struct monsteri;

struct creaturei : npc, statable, levelable, wearable, posable {
	statable		basic;
	short			hp, hpm, hpr;
	short unsigned	monster_id;
	char			initiative;
	spella			spells;
	racef			hate;
	flag32			languages;
	void			add(abilityn i, int v);
	void			addexp(int value);
	void			additem(item& it);
	bool			canread() const { return get(Intellegence) >= 9; }
	void			clear();
	void			damage(damagen type, int hits, char magic_bonus = 0);
	int				get(abilityn v) const { return abilities[v]; }
	const char*		getbadstate() const;
	int				getchance(abilityn v) const;
	const classi&	getclass() const;
	const classi&	getclassmain() const;
	dice			getdamage(int& bonus, wearn id, bool large_enemy) const;
	int				getexpaward() const;
	int				gethitpenalty(int bonus) const;
	const monsteri*	getmonster() const;
	const char*		getname() const;
	const racei&	getrace() const;
	void			heal(int value);
	bool			is(abilityn v) const { return abilities[v] > 0; }
	bool			is(featn v) const { return featable::is(v); }
	bool			is(alignmentn v) const { return alignment == v; }
	bool			is(const item& weapon, featn v) const;
	bool			isactable() const;
	bool			isallow(const item& it) const;
	bool			isdead() const { return hp <= -10; }
	bool			isdisabled() const { return hp <= 0; }
	bool			ismonster() const { return getmonster() != 0; }
	bool			isunderstand(racen v) const { return languages.is(v); }
	void			kill();
	void			remove(featn v) { featable::remove(v); }
	bool			roll(abilityn v, int bonus = 0) const;
	void			set(featn v) { featable::set(v); }
	void			setframe(short* frames, short index) const;
	bool			useammo();
	void			understand(racen v) { languages.set(v); }
};
extern creaturei *player, *opponent;
extern int last_roll, last_chance;
extern bool is_critical_hit;

void add_spells(int type, int level, const spellseta* include);
void create_player();
void create_monster(const monsteri* pi);
void update_player();

creaturei* item_owner(const void* p);
wearn item_wear(const void* p);
bool can_remove(const item* pi, bool speech = true);