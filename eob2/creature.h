#pragma once

#include "statable.h"
#include "levelable.h"

enum racen : unsigned char;
enum gendern : unsigned char;
enum classn : unsigned char;

struct creaturei : statable, levelable {
	statable		basic;
	short			hp, hpm;
	racen			race;
	classn			type;
	gendern			gender;
	unsigned char	avatar = 0xFF;
	int				get(abilityn v) const { return abilities[v]; }
};
extern creaturei* player;

void create_player(racen race, gendern gender, classn cls);
void update_player();