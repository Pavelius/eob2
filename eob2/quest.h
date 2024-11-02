#pragma once

#include "dungeon_site.h"
#include "goal.h"

struct quest : nameable {
	dungeon_site	sites[8];
};
extern quest* last_quest;