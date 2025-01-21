#include "adat.h"
#include "quest.h"
#include "party.h"
#include "rand.h"
#include "script.h"

typedef adat<quest*> questc;

quest* last_quest;
quest* party_quests[128];

void clear_quests() {
	memset(party_quests, 0, sizeof(party_quests));
}

unsigned short find_quest(quest* p) {
	if(!p)
		return 0xFFFF;
	for(size_t i = 0; i < sizeof(party_quests) / sizeof(party_quests[0]); i++) {
		if(party_quests[i] == p)
			return (unsigned short)i;
	}
	return 0xFFFF;
}

unsigned short add_quest(quest* p) {
	if(!p)
		return 0xFFFF;
	auto index = find_quest(p);
	if(index != 0xFFFF)
		return index;
	for(auto& e : party_quests) {
		if(e)
			continue;
		e = p;
		index = &e - party_quests;
		break;
	}
	return index;
}

static void add_quests(const questc& source) {
	for(auto p : source)
		add_quest(p);
}

static void select_quests(questc& source, int difficult) {
	source.clear();
	for(auto& e : bsdata<quest>()) {
		if(e.difficult != difficult)
			continue;
		source.add(&e);
	}
}

static void prepare_quests(questc& source) {
	auto current = source.getcount();
	if(current < 2)
		return;
	source.shuffle();
	source.top(current / 2); // Only half quest used. Other quest can be in next started game.
}

static void activate_quest(quest* p) {
	ftscript<quest>(getbsi(p), 0);
}

void create_game_quests() {
	questc source;
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
	auto quest_id = find_quest(last_quest);
	if(quest_id == 0xFFFF)
		return false;
	for(auto i = (goaln)0; i <= KillAlmostAllMonsters; i = (goaln)(i + 1)) {
		if(!last_quest->goals[i])
			continue;
		auto value = party_goal(quest_id, i);
		if(value < last_quest->goals[i])
			return false;
	}
	return true;
}