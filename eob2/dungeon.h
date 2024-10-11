#pragma once

#include "creature.h"
#include "dungeon_site.h"
#include "dungeon_state.h"
#include "item.h"
#include "slice.h"

const int mpx = 38;
const int mpy = 23;

enum celln : unsigned char;
enum cellfn : unsigned char;

struct dungeoni : dungeon_site {
	struct ground : item, pointc {
	};
	dungeon_state	state;
	ground			items[512];
	creaturei		monsters[200];
	unsigned char	data[mpy][mpx];
	void			block(bool treat_door_as_passable) const;
	celln			get(pointc v) const;
	slice<ground>	getitems() { return slice<ground>(items, state.items); }
	slice<creaturei> getmonsters() { return slice<creaturei>(monsters, state.monsters); }
	bool			is(pointc v, cellfn i) const;
	void			remove(pointc v, cellfn i);
	void			set(pointc v, celln i);
	void			set(pointc v, celln i, pointc size);
	void			set(pointc v, cellfn i);
};
extern dungeoni *loc, *locup, *locdw;
