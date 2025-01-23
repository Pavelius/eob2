#pragma once

#include "adat.h"
#include "dungeon_site.h"
#include "goal.h"
#include "variant.h"

struct dungeoni;

enum questf : char {
	QuestActive, QuestClosed,
};
struct quest : nameable {
	struct leveli : dungeon_site {
		variants features;
	};
	struct historyi {
		short unsigned monster;
		char stage, value;
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
	historyi*	addhistory(unsigned short id);
	void		clear();
	historyi*	findhistory(unsigned short id) const;
	int			gethistory(unsigned short id) const;
	bool		is(questf v) const { return flags.is(v); }
	void		prepare();
	void		remove(questf v) { flags.remove(v); }
	void		set(questf v) { flags.set(v); }
};
extern quest* last_quest;

void create_game_quests();
bool last_quest_complite();