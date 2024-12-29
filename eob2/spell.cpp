#include "answers.h"
#include "boost.h"
#include "bsdata.h"
#include "creature.h"
#include "direction.h"
#include "dungeon.h"
#include "formula.h"
#include "math.h"
#include "modifier.h"
#include "party.h"
#include "pushvalue.h"
#include "rand.h"
#include "script.h"
#include "spell.h"
#include "view.h"

creaturei* caster;
const spelli* last_spell;

static void apply_summon(creaturei* player, const itemi* pi) {
	auto wear = pi->wear;
	if(player->wears[wear])
		return;
	player->wears[wear].create(pi);
	player->wears[wear].identify(1);
	player->wears[wear].summon(1);
}

template<> void ftscript<spelli>(int value, int bonus) {
	spellseta* ps;
	switch(modifier) {
	case Standart:
		if(bsdata<spelli>::elements[value].summon)
			apply_summon(player, bsdata<spelli>::elements[value].summon);
		script_run(bsdata<spelli>::elements[value].wearing);
		break;
	case Permanent:
		ps = get_spells_known(player);
		if(ps) {
			if(bonus >= 0)
				ps->set(value);
			else
				ps->remove(value);
		}
		break;
	default:
		break;
	}
}

static spelli* choose_prepared_spell() {
	pushanswer push_answer;
	for(auto& e : bsdata<spelli>()) {
		auto index = &e - bsdata<spelli>::elements;
		auto count = player->spells[index];
		for(auto i = 0; i < count; i++)
			an.add(&e, e.getname());
	}
	if(!an) {
		auto caster = player->getclass().caster;
		if(caster == -1)
			player->speak("CastSpell", "NoCaster");
		else
			player->speak("CastSpell", "NoSpells");
		return 0;
	}
	an.sort();
	return (spelli*)choose_small_menu(getnm("CastSpell"), getnm("Cancel"));
}

static void filter_creatures(const variants& source) {
	if(!source)
		return;
	auto ps = an.elements.begin();
	auto push_player = player;
	for(auto& e : an.elements) {
		player = (creaturei*)e.value;
		if(!script_allow(source))
			continue;
		*ps++ = e;
	}
	player = push_player;
	an.elements.count = ps - an.begin();
}

static void remove_summon_slot(wearn wear) {
	for(auto& e : an.elements) {
		auto player = (creaturei*)e.value;
		if(!player->wears[wear])
			continue;
		if(!can_remove(&player->wears[wear], false))
			continue;
		item* ps = 0;
		if(wear == RightHand || wear == LeftHand)
			ps = player->freebelt();
		else
			ps = player->freebackpack();
		if(!ps)
			continue;
		iswap(*ps, player->wears[wear]);
	}
}

static void filter_summon_slot(wearn wear) {
	auto ps = an.elements.begin();
	auto push_player = player;
	for(auto& e : an.elements) {
		player = (creaturei*)e.value;
		if(player->wears[wear]) {
			if(!can_remove(&player->wears[wear], false))
				continue;
			if(wear == RightHand || wear == LeftHand) {
				if(!player->freebelt())
					continue;
			}
		}
		*ps++ = e;
	}
	player = push_player;
	an.elements.count = ps - an.begin();
}

static void add_targets(pointc v, bool enemy, bool ally, bool include_player) {
	creaturei* targets[6] = {};
	if(enemy) {
		if(loc)
			loc->getmonsters(targets, v);
	} else
		memcpy(targets, characters, sizeof(targets));
	for(auto p : targets) {
		if(!p)
			continue;
		if(p == player && !include_player)
			continue;
		an.add(p, p->getname());
	}
}

static bool strict_value(const array& source, const void* p) {
	auto i = source.indexof(p);
	if(i == -1)
		return false;
	return source.ptr(i) == p;
}

static void apply_effect(const variants& source) {
	if(!source)
		return;
	auto push_player = player;
	auto push_item = last_item;
	for(auto& e : an) {
		auto p = e.value;
		player = item_owner(p);
		if(player) {
			last_item = (item*)p;
			script_run(source);
		} else {
			player = (creaturei*)p;
			script_run(source);
		}
	}
	last_item = push_item;
	player = push_player;
}

void apply_enchant_spell(int bonus) {
	last_number += bonus;
	if(!last_spell || last_number <= 0 || !player)
		return;
	add_boost(party.abilities[Minutes] + last_number, player, last_spell);
}

static void select_items(const variants& filter) {
	auto ans = an;
	an.clear();
	auto push_item = last_item;
	for(auto& e : ans) {
		auto player = (creaturei*)e.value;
		for(auto& it : player->wears) {
			if(!it)
				continue;
			last_item = &it;
			if(!script_allow(filter))
				continue;
			an.add(&it, it.getname());
		}
	}
	last_item = push_item;
}

static bool choose_single(const char* title) {
	creaturei* target = 0;
	target = (creaturei*)choose_small_menu(title, "Cancel");
	if(!target)
		return false;
	an.clear();
	an.addv(target, target->getname(), 0, '1');
	return true;
}

static bool look_group(pointc& v, directions d) {
	if(!loc)
		return false;
	for(auto i = 0; i < 3; i++) {
		v = to(v, d);
		if(!v || !loc->ispassable(v))
			return false;
		if(v == party || loc->ismonster(v))
			return true;
	}
	return false;
}

static int distance(pointc v1, pointc v2) {
	if(!v1 || !v2)
		return 0;
	auto dx = iabs(v1.x - v2.x);
	auto dy = iabs(v1.y - v2.y);
	return dx > dy ? dx : dy;
}

static void use_spell_slot(const spelli* ps) {
	auto index = getbsi(ps);
	if(index == -1)
		return;
	if(player->spells[index])
		player->spells[index]--;
}

bool cast_spell(const spelli* ps, int level, int experience, bool run, bool random_target) {
	auto push_spell = last_spell;
	pushanswer push_answers;
	pointc enemy_position = to(party, party.d);
	last_spell = ps;
	result_player = 0;
	auto need_creatures = ps->is(Ally) || ps->is(Enemy) || ps->is(You);
	if(need_creatures) {
		if(ps->is(Ally))
			add_targets(party, false, true, ps->is(You));
		if(ps->is(Enemy)) {
			if(ps->isthrown()) {
				enemy_position = party;
				if(look_group(enemy_position, party.d))
					add_targets(enemy_position, true, false, false);
			} else {
				enemy_position = to(party, party.d);
				add_targets(enemy_position, true, false, ps->is(You));
			}
		}
		if(!ps->is(Ally) && !ps->is(Enemy) && ps->is(You))
			an.add(player, player->getname());
		if(ps->summon)
			filter_summon_slot(ps->summon->wear);
		if(!ps->is(WearItem))
			filter_creatures(ps->filter);
		if(!an) {
			if(run)
				player->speak("CastSpell", "NoTargets");
			return false;
		}
		if(!run)
			return true;
		if(!ps->is(Group)) {
			if(ps->is(Ally) && !ps->is(Enemy) && !random_target) {
				if(!choose_single(getnm("CastOnWho")))
					return false;
			} else {
				zshuffle(an.elements.data, an.elements.count);
				an.elements.count = 1;
			}
		}
		if(an)
			result_player = (creaturei*)an.elements[0].value;
		if(ps->is(WearItem))
			select_items(ps->filter);
	} else if(!ps->is(WearItem)) {
		if(!script_allow(ps->filter))
			return false;
		if(!run)
			return true;
	}
	if(ps->isthrown()) {
		auto n = distance(party, enemy_position);
		thrown_item(party, Up, ps->avatar_thrown, player->side % 2, n);
	}
	player->addexp(experience);
	auto push_caster = caster; caster = player;
	auto push_level = last_level; last_level = level;
	auto push_id = last_id; last_id = last_spell->id;
	party.abilities[EffectCount] = 0;
	if(need_creatures) {
		apply_effect(ps->instant);
		if(ps->summon)
			remove_summon_slot(ps->summon->wear);
	} else
		script_run(ps->instant);
	fix_animate();
	if(party.abilities[EffectCount])
		player->speakn(ps->id, "GainEffect", party.abilities[EffectCount]);
	else
		player->speakn(ps->id, "NoEffect", party.abilities[EffectCount]);
	last_id = push_id;
	last_spell = push_spell;
	last_level = push_level;
	caster = push_caster;
	return true;
}

void cast_spell() {
	auto ps = choose_prepared_spell();
	if(!ps)
		return;
	// RULE: add experience for each spell cast.
	if(!cast_spell(ps, player->getlevel(), 35, true, false))
		return;
	use_spell_slot(ps);
	pass_round();
}

void add_spells(int type, int level, const spellseta* include) {
	an.clear();
	for(auto& e : bsdata<spelli>()) {
		if(e.levels[type] != level)
			continue;
		auto index = getbsi(&e);
		if(include && !include->is(index))
			continue;
		an.add(&e, e.getname());
	}
}
