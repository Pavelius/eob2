#pragma once

#include "statable.h"
#include "levelable.h"
#include "wearable.h"

enum racen : unsigned char;
enum gendern : unsigned char;

struct classi;
struct racei;

struct creaturei : statable, levelable, wearable {
	statable		basic;
	short			hp, hpm, hpr;
	racen			race;
	char			type;
	gendern			gender;
	unsigned short	name;
	unsigned char	avatar;
	void			clear();
	int				get(abilityn v) const { return abilities[v]; }
	const char*		getname() const;
	bool			isdead() const { return hp <= -10; }
	bool			isdisabled() const { return hp <= 0; }
};
extern creaturei* player;

void create_player(const racei* pr, gendern gender, const classi* pc);
void update_player();