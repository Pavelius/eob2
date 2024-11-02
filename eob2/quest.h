#pragma once

#include "dungeon_site.h"
#include "goal.h"

struct quest : nameable {
	goala			goals;
	dungeon_site	sites[8];
};
extern quest* last_quest;