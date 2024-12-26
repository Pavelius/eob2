#pragma once

#include "class.h"
#include "damage.h"
#include "statable.h"
#include "posable.h"
#include "race.h"
#include "reaction.h"
#include "spell.h"
#include "npc.h"
#include "wearable.h"

struct monsteri;

struct creaturei : npc, statable, wearable, posable {
	statable		basic;
	short			hp, hpm, hpr, hp_aid;
	short unsigned	monster_id;
	char			initiative;
	spella			spells;
	racef			hate;
	flag32			languages;
	reactions		reaction;
	void			add(abilityn i, int v);
	void			addexp(int value);
	void			additem(item& it);
	bool			canread() const { return get(Intellegence) >= 9; }
	void			clear();
	void			damage(damagen type, int hits, char magic_bonus = 0);
	int				get(abilityn v) const { return abilities[v]; }
	int				getcaster() const;
	const char*		getbadstate() const;
	int				getchance(abilityn v) const;
	dice			getdamage(int& bonus, wearn id, bool large_enemy) const;
	int				getexpaward() const;
	int				gethitpenalty(int bonus) const;
	int				gethp() const { return hp + hp_aid; }
	const monsteri*	getmonster() const;
	const char*		getname() const;
	void			heal(int value);
	bool			is(abilityn v) const { return abilities[v] > 0; }
	bool			is(featn v) const { return featable::is(v); }
	bool			is(alignmentn v) const { return alignment == v; }
	bool			is(const item& weapon, featn v) const;
	bool			isactable() const;
	bool			isallow(const item& it) const;
	bool			isanimal() const { return get(Intellegence) <= 4; }
	bool			isdead() const { return gethp() <= -10; }
	bool			isdisabled() const { return gethp() <= 0; }
	bool			ismonster() const { return getmonster() != 0; }
	bool			isunderstand(racen v) const { return languages.is(v); }
	bool			isready() const;
	void			kill();
	void			remove(featn v) { featable::remove(v); }
	bool			roll(abilityn v, int bonus = 0) const;
	void			set(featn v) { featable::set(v); }
	void			setframe(short* frames, short index) const;
	void			understand(racen v) { languages.set(v); }
};
extern creaturei *player, *opponent, *caster;
extern int last_roll, last_chance;
extern bool is_critical_hit;

void add_spells(int type, int level, const spellseta* include);
void check_levelup();
void create_player();
void create_monster(const monsteri* pi);
void drop_unique_loot(creaturei* player);
void set_reaction(creaturei** creatures, reactions v);
void update_player();

creaturei* item_owner(const void* p);
wearn item_wear(const void* p);
bool can_remove(item* pi, bool speech = true);