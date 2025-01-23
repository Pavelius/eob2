#include "adat.h"
#include "dungeon.h"
#include "quest.h"
#include "party.h"
#include "rand.h"
#include "script.h"

quest* last_quest;

typedef adat<quest*> questa;

quest::historyi* quest::findhistory(unsigned short id) const {
	for(auto& e : history) {
		if(e.monster == id)
			return const_cast<historyi*>(&e);
	}
	return 0;
}

quest::historyi* quest::addhistory(unsigned short id) {
	auto p = findhistory(id);
	if(!p) {
		p = history.add();
		p->monster = id;
		p->value = 0;
		p->stage = 0;
	}
	return p;
}

int quest::gethistory(unsigned short id) const {
	auto p = findhistory(id);
	if(!p)
		return 0;
	return p->value;
}

void quest::clear() {
	flags.clear();
	dungeon.clear();
	history.clear();
}

void quest::prepare() {
	if(dungeon)
		return;
	auto push = last_quest;
	last_quest = this;
	dungeon.setbegin();
	dungeon_create();
	dungeon.setend();
	last_quest = push;
}

static void clear_quests() {
	for(auto& e : bsdata<quest>())
		e.clear();
}

//unsigned short find_quest(quest* p) {
//	if(!p)
//		return 0xFFFF;
//	return party_quests.find(p);
//}
//
//unsigned short add_quest(quest* p) {
//	if(!p)
//		return 0xFFFF;
//	auto index = find_quest(p);
//	if(index != 0xFFFF)
//		return index;
//	auto pn = party_quests.add();
//	*pn = p;
//	return pn - party_quests.data;
//}
//
//static void add_quests(const questc& source) {
//	for(auto p : source)
//		add_quest(p);
//}

static void select_quests(questa& source, int difficult) {
	source.clear();
	for(auto& e : bsdata<quest>()) {
		if(e.difficult != difficult)
			continue;
		source.add(&e);
	}
}

static void prepare_quests(questa& source) {
	auto current = source.getcount();
	if(current < 2)
		return;
	source.shuffle();
	source.top(current / 2); // Only half quest used. Other quest can be in next started game.
}

static void add_quests(questa& source) {
	for(auto p : source)
		p->prepare();
}

void create_game_quests() {
	questa source;
	clear_quests();
	for(auto i = 0; i <= 5; i++) {
		select_quests(source, i);
		prepare_quests(source);
		add_quests(source);
	}
}

bool last_quest_complite() {
	if(!last_quest)
		return false;
	for(auto i = (goaln)0; i <= KillAlmostAllMonsters; i = (goaln)(i + 1)) {
		if(!last_quest->goals[i])
			continue;
		auto value = party_goal(i);
		if(value < last_quest->goals[i])
			return false;
	}
	return true;
}