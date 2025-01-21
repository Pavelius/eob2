#pragma once

#include "dungeon_site.h"
#include "goal.h"
#include "variant.h"

struct dungeoni;

struct quest : nameable {
	struct leveli : dungeon_site {
		variants features;
	};
	char		difficult; // From 0 to 5. Add to magic item level.
	goala		goals;
	variants	travel, reward, reward_history;
	leveli		sites[8];
};
extern quest* last_quest;
extern quest* party_quests[128];

void clear_quests();
void create_game_quests();
bool last_quest_complite();

unsigned short add_quest(quest* p);
unsigned short find_quest(quest* p);