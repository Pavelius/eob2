#pragma once

#include "statable.h"
#include "levelable.h"

struct creaturei : statable, levelable {
	statable	basic;
	short		hp, hpm;
	int			get(abilityn v) const { return abilities[v]; }
};
extern creaturei* player;

void update_player();