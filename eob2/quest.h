#pragma once

#include "dungeon_site.h"
#include "goal.h"
#include "variant.h"

struct dungeoni;

struct quest : nameable {
	goala			goals;
	variants		travel, reward;
	dungeon_site	sites[8];
};
extern quest* last_quest;