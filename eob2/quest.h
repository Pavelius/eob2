#pragma once

#include "adat.h"
#include "dungeon_site.h"
#include "goal.h"
#include "variant.h"

struct dungeoni;

struct quest : nameable {
	enum stagen : unsigned char {
		Hide, Active, Done,
	};
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
	stagen		stage;
	leveli		sites[8];
	historya	history;
	dungeona	dungeon;
	explicit operator bool() const { return dungeon.operator bool(); }
	historyi*	addhistory(unsigned short id);
	void		clear();
	historyi*	findhistory(unsigned short id) const;
	int			gethistory(unsigned short id) const;
	void		prepare();
};
extern quest* last_quest;

void create_game_quests();
bool last_quest_complite();