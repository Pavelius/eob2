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

enum boostn : short {
	BoostSpell = 1000, BoostFeat,
};
struct creaturei : npc, statable, wearable, posable {
	statable		basic;
	short			hp, hpm, hpr, hp_aid, food;
	short unsigned	monster_id;
	char			initiative, pallette;
	spella			spells;
	racef			hate;
	flag32			languages;
	reactions		reaction;
	explicit operator bool() const { return abilities[Strenght] != 0; }
	void			add(abilityn i, int v);
	void			addexp(int value);
	void			additem(item& it);
	bool			canlearnspell(int type, int level) const;
	bool			canread() const { return get(Intellegence) >= 9; }
	void			clear();
	void			damage(damagen type, int hits, char magic_bonus = 0);
	int				get(abilityn v) const { return abilities[v]; }
	int				getcaster() const;
	const char*		getbadstate() const;
	int				getchance(abilityn v) const;
	dice			getdamage(int& bonus, wearn id, bool large_enemy) const;
	int				getexpaward() const;
	int				getfood() const;
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
	void			set(featn v, bool apply) { featable::set(v, apply); }
	void			setframe(short* frames, short index) const;
	void			understand(racen v) { languages.set(v); }
};
extern creaturei *player, *opponent, *caster, *result_player;
extern bool is_critical_hit;

void add_spells(int type, int level, const spellseta* include);
void apply_player_script(const char* action);
bool can_loose(item* pi, bool speech = true);
bool can_remove(item* pi, bool speech = true);
void change_quick_item(creaturei* player, wearn w);
void check_levelup();
void create_player();
void create_player_super_stats();
void create_player_finish();
void create_monster(const monsteri* pi);
void create_monster_pallette();
void drop_unique_loot(creaturei* player);
void generate_abilities();
bool is_party_name(unsigned short value);
bool no_party_avatars(unsigned char value);
bool roll_ability(int chance);
void roll_player_hits();
void set_monster_spells();
void apply_race_ability();
void set_reaction(creaturei** creatures, reactions v);
void update_player();
void update_player_hits();

creaturei* item_owner(const void* p);
wearn item_wear(const void* p);