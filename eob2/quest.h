#pragma once

#include "adat.h"
#include "dungeon_site.h"
#include "goal.h"
#include "variant.h"

struct dungeoni;

enum questf : char {
	QuestActive, QuestPrepared, QuestClosed,
};
struct quest : nameable {
	struct leveli : dungeon_site {
		variants features;
	};
	struct historyi {
		short unsigned	monster;
		char			stage, value;
	};
	typedef adat<historyi, 8> historya;
	typedef sliceu<dungeoni> dungeona;
	char		difficult; // From 0 to 5. Add to magic item level.
	goala		goals;
	variants	travel, reward;
	flag8		flags;
	leveli		sites[8];
	historya	history;
	dungeona	dungeon;
	bool		is(questf v) const { return flags.is(v); }
	void		remove(questf v) { flags.remove(v); }
	void		set(questf v) { flags.set(v); }
};
extern quest* last_quest;
extern adat<quest*> party_quests;

void clear_quests();
void create_game_quests();
bool last_quest_complite();

unsigned short add_quest(quest* p);
unsigned short find_quest(quest* p);