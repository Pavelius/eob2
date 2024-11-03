#pragma once

#include "dungeon_site.h"
#include "goal.h"
#include "variant.h"

struct quest : nameable {
	goala			goals;
	variants		travel;
	dungeon_site	sites[8];
};
extern quest* last_quest;