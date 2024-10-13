#pragma once

#include "dungeon_site.h"
#include "nameable.h"

struct quest : nameable {
	dungeon_site	sites[8];
};
