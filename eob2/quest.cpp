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
	stage = Hide;
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

static void select_quests(questa& source, int difficult) {
	source.clear();
	for(auto& e : bsdata<quest>()) {
		if(e.difficult != difficult)
			continue;
		source.add(&e);
	}
}

static void prepare_quests(questa& source) {
	auto current = source.getcount() / 2;
	if(current < 1)
		return;
	if(current > 4)
		current = 4;
	source.shuffle();
	source.top(current); // Only half quest used. Other quest can be in next started game.
}

static void add_quests(questa& source) {
	for(auto p : source)
		p->prepare();
}

void create_game_quests() {
	questa source;
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