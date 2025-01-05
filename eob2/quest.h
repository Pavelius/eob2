#pragma once

#include "dungeon_site.h"
#include "goal.h"
#include "variant.h"

struct dungeoni;

struct quest : nameable {
	struct leveli : dungeon_site {
		variants features;
	};
	goala goals;
	variants travel, reward;
	leveli sites[8];
};
extern quest* last_quest;

bool last_quest_complite();