#include "answers.h"
#include "boost.h"
#include "bsdata.h"
#include "cell.h"
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

int	spelli::getindex() const {
	return getbsi(this);
}

static void apply_summon(creaturei* player, const itemi* pi) {
	auto wear = pi->wear;
	if(player->wears[wear])
		return;
	player->wears[wear].create(pi);
	player->wears[wear].identify(1);
	player->wears[wear].set(SummonedItem);
}

template<> bool fttest<spelli>(int value, int bonus) {
	spellseta* ps;
	switch(modifier) {
	case Permanent:
		ps = get_spells_known(player);
		return ps ? ps->is(value) : false;
	default:
		return player->spells[value] > 0;
	}
}

template<> void ftscript<spelli>(int value, int bonus) {
	spellseta* ps;
	switch(modifier) {
	case Standart:
		last_spell = bsdata<spelli>::elements + value;
		player->spells[value] += bonus;
		break;
	case Wearing:
		if(bsdata<spelli>::elements[value].summon)
			apply_summon(player, bsdata<spelli>::elements[value].summon);
		script_run(bsdata<spelli>::elements[value].wearing);
		break;
	case Permanent:
		ps = get_spells_known(player);
		if(ps)
			ps->set(value, bonus >= 0);
		break;
	default:
		break;
	}
}

static bool is_enchant(referencei target, variant v) {
	for(auto& e : bsdata<boosti>()) {
		if(e.target == target && e.effect.type == v.type && e.effect.value == v.value)
			return true;
	}
	return false;
}

static spelli* choose_prepared_spell() {
	pushanswer push_answer;
	if(!player->isactable())
		return 0;
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

static bool have_items(creaturei* player, const variants& source) {
	pushvalue push(last_item);
	for(auto& e : player->wears) {
		if(!e)
			continue;
		last_item = &e;
		if(script_allow(source))
			return true;
	}
	return false;
}

static void filter_creature_items(const variants& source) {
	if(!source)
		return;
	pushvalue push(player);
	auto ps = an.elements.begin();
	for(auto& e : an.elements) {
		if(!have_items((creaturei*)e.value, source))
			continue;
		*ps++ = e;
	}
	an.elements.count = ps - an.begin();
}

static void remove_summon_slot(wearn wear) {
	if(!player->wears[wear])
		return;
	if(!can_remove(&player->wears[wear], false))
		return;
	item* ps = 0;
	if(wear == RightHand || wear == LeftHand)
		ps = player->freebelt();
	else
		ps = player->freebackpack();
	if(!ps)
		return;
	iswap(*ps, player->wears[wear]);
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
		if(is_enchant(p, last_spell))
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

static void apply_effect(const variants& source, const itemi* summon) {
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
			if(summon)
				remove_summon_slot(summon->wear);
		}
		update_player();
	}
	last_item = push_item;
	player = push_player;
}

void apply_enchant_spell(int bonus) {
	last_number += bonus;
	if(last_spell && last_number > 0 && player)
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

static bool use_spell_on_object(pointc v, bool run) {
	if(!v)
		return false;
	if(!last_spell->filter_cell)
		return false;
	auto t = loc->get(v);
	if(!last_spell->filter_cell.is(t))
		return false;
	if(run) {
		loc->broke(v);
		animation_update();
	}
	return true;
}

bool cast_spell(const spelli* ps, int level, int experience, bool run, bool random_target, unsigned durations, creaturei* explicit_target) {
	pushvalue push_spell(last_spell, ps);
	pushanswer push_answers;
	pointc enemy_position = to(party, party.d);
	result_player = 0;
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
	if(ps->is(You)) {
		if(an.findvalue(player) == -1)
			an.add(player, player->getname());
	}
	if(explicit_target && !ps->is(Group)) {
		an.clear();
		an.add(explicit_target, explicit_target->getname());
	}
	if(ps->summon)
		filter_summon_slot(ps->summon->wear);
	filter_creatures(ps->filter);
	if(ps->filter_item)
		filter_creature_items(ps->filter_item);
	if(!an && !use_spell_on_object(enemy_position, false)) {
		if(run)
			player->speak("CastSpell", "NoTargets");
		return false;
	}
	if(!run)
		return true;
	player->addexp(experience);
	if(an && !ps->is(Group)) {
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
	if(ps->isthrown()) {
		auto n = distance(party, enemy_position);
		thrown_item(party, Up, ps->avatar_thrown, player->side % 2, n);
	}
	if(use_spell_on_object(enemy_position, true))
		return true;
	pushvalue push_caster(caster, player);
	pushvalue push_level(last_level, level);
	pushvalue push_id(last_id, last_spell->id);
	party.abilities[EffectCount] = 0;
	if(!durations && ps->duration)
		durations = ps->duration->roll(last_level);
	last_number = durations;
	if(ps->is(SummaryEffect))
		script_run(ps->instant);
	else {
		if(ps->filter_item)
			select_items(ps->filter_item);
		apply_effect(ps->instant, ps->summon);
	}
	fix_animate();
	if(party.abilities[EffectCount])
		apply_message(ps->id, "GainEffect");
	else
		apply_message(ps->id, "NoEffect");
	return true;
}

void cast_spell() {
	auto ps = choose_prepared_spell();
	if(!ps)
		return;
	// RULE: add experience for each spell cast.
	if(!cast_spell(ps, player->getlevel(), 35, true, false, 0, 0))
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

static int get_spells_count(spellseta* spells, int type, int level) {
	auto result = 0;
	for(auto& e : bsdata<spelli>()) {
		if(e.levels[type] != level)
			continue;
		if(spells->is(e.getindex()))
			result++;
	}
	return result;
}

bool can_cast_spell(int type, int level) {
	static char maximum_spell_level[] = {
		0, 1, 1, 1, 1, 2, 2, 2, 3, 4,
		5, 5, 6, 6, 7, 7, 8, 8, 9
	};
	int ability;
	switch(type) {
	case 1:
		ability = player->get(Intellegence);
		return maptbl(maximum_spell_level, ability) >= level;
	default:
		return true;
	}
}

bool can_learn_spell(int type, int level) {
	static char maximum_number_of_spells[] = {
		1, 1, 1, 1, 2, 3, 4, 5, 6, 6,
		7, 7, 8, 8, 9, 9, 10, 11, 12
	};
	auto ps = get_spells_known(player);
	if(!ps)
		return false;
	if(!can_cast_spell(type, level))
		return false;
	if(type == 1) {
		auto intellegence = player->basic.abilities[Intellegence];
		auto exist_count = get_spells_count(ps, type, level);
		auto maximum_count = maptbl(maximum_number_of_spells, intellegence);
		if(exist_count >= maximum_count)
			return false;
	}
	return true;
}