#pragma once

#include "creature.h"
#include "dungeon_site.h"
#include "dungeon_state.h"
#include "item.h"
#include "slice.h"

enum celln : unsigned char;
enum cellfn : unsigned char;

typedef void (*fncorridor)(pointc v, directions d, unsigned flags);

struct dungeoni : dungeon_site {
	struct ground : item, pointc {
	};
	struct overlayi : pointc {
		celln			type; // type of overlay
		pointc			link; // linked to this location
		short unsigned	subtype; // depends on value type
		void			clear();
	};
	dungeon_state	state;
	ground			items[512];
	creaturei		monsters[200];
	unsigned char	data[mpy][mpx];
	overlayi*		add(pointc v, celln i, directions d);
	void			addmonster(pointc v, unsigned char type) {}
	void			block(bool treat_door_as_passable) const;
	celln			get(pointc v) const;
	slice<ground>	getitems() { return slice<ground>(items, state.items); }
	slice<creaturei> getmonsters() { return slice<creaturei>(monsters, state.monsters); }
	overlayi*		getoverlay(pointc v, directions d);
	bool			is(pointc v, cellfn i) const;
	bool			is(pointc v, celln t1, celln t2) const;
	void			remove(pointc v, cellfn i);
	void			set(pointc v, celln i);
	void			set(pointc v, celln i, directions d);
	void			set(pointc v, celln i, pointc size);
	void			set(pointc v, cellfn i);
};
extern dungeoni *loc, *locup, *locdw;

void dungeon_create();
