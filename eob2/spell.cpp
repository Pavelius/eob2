#include "answers.h"
#include "boost.h"
#include "bsdata.h"
#include "creature.h"
#include "dungeon.h"
#include "formula.h"
#include "modifier.h"
#include "party.h"
#include "script.h"
#include "spell.h"
#include "view.h"

template<> void ftscript<spelli>(int value, int bonus) {
	switch(modifier) {
	case Standart:
		script_run(bsdata<spelli>::elements[value].wearing);
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
	return (spelli*)choose_small_menu(getnm("CastSpell"), getnm("Cancel"));
}

static void filter_creatures(const variants& source) {
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

static void add_targets(bool enemy, bool ally, bool include_player) {
	creaturei* targets[6] = {};
	if(enemy)
		loc->getmonsters(targets, to(party, party.d));
	else
		memcpy(targets, characters, sizeof(targets));
	for(auto p : targets) {
		if(!p)
			continue;
		if(p == player && !include_player)
			continue;
		an.add(p, p->getname());
	}
}

static void apply_effect(const variants& source) {
	if(!source)
		return;
	auto push_player = player;
	for(auto& e : an) {
		auto p = e.value;
		if(bsdata<creaturei>::source.have(p) || (loc && loc->have((creaturei*)p))) {
			player = (creaturei*)p;
			script_run(source);
		}
	}
	player = push_player;
}

static void apply_enchant_effect(const randomeffecti* duration, int level, variant action) {
	if(!action || !duration)
		return;
	auto rounds = duration->roll(level);
	auto push_player = player;
	for(auto& e : an) {
		auto p = e.value;
		if(bsdata<creaturei>::source.have(p) || (loc && loc->have((creaturei*)p)))
			add_boost(party.abilities[Minutes] + rounds, (creaturei*)p, action);
	}
	player = push_player;
}

static bool cast_spell(const spelli* ps, bool run) {
	pushanswer push_answers;
	if(ps->is(Ally))
		add_targets(false, true, ps->is(You));
	if(ps->is(Enemy))
		add_targets(true, false, ps->is(You));
	if(ps->is(You) || ps->is(Ally) || ps->is(Enemy)) {
		if(ps->filter)
			filter_creatures(ps->filter);
	}
	if(!an) {
		if(run)
			player->speak("CastSpell", "NoTargets");
		return false;
	}
	if(!run)
		return true;
	if(!ps->is(Group)) {
		auto target = (creaturei*)choose_small_menu(getnm("CastOnWho"), "Cancel");
		if(!target)
			return false;
		an.clear();
		an.addv(target, target->getname(), 0, '1');
	}
	auto level = player->getlevel();
	if(ps->effect)
		last_number = ps->effect->roll(level);
	party.abilities[EffectCount] = 0;
	apply_effect(ps->instant);
	apply_enchant_effect(ps->duration, level, ps);
	if(party.abilities[EffectCount])
		player->speakn(ps->id, "GainEffect", party.abilities[EffectCount]);
	else
		player->speakn(ps->id, "NoEffect", party.abilities[EffectCount]);
	return true;
}

void cast_spell() {
	auto ps = choose_prepared_spell();
	if(!ps)
		return;
	if(!cast_spell(ps, true))
		return;
	auto index = getbsi(ps);
	if(index == -1)
		return;
	if(player->spells[index])
		player->spells[index]--;
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