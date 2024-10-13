#pragma once

#include "creature.h"
#include "dungeon_site.h"
#include "dungeon_state.h"
#include "item.h"
#include "slice.h"

enum celln : unsigned char;
enum cellfn : unsigned char;

struct pointca;

struct dungeoni : dungeon_site {
	struct ground : item, posable {
		void		clear();
	};
	struct overlayi : posable {
		celln		type; // type of overlay
		pointc		link; // linked to this location
		unsigned char subtype; // depends on value type
		void		clear();
	};
	dungeon_state	state;
	ground			items[512];
	creaturei		monsters[200];
	overlayi		overlays[255];
	unsigned char	data[mpy][mpx];
	overlayi*		add(pointc v, celln i, directions d);
	void			addmonster(pointc v, unsigned char type) {}
	int				around(pointc v, celln t1, celln t2) const;
	void			block(bool treat_door_as_passable) const;
	void			clear();
	void			change(celln s, celln n);
	celln			get(pointc v) const;
	slice<ground>	getitems() { return slice<ground>(items, state.items); }
	slice<creaturei> getmonsters() { return slice<creaturei>(monsters, state.monsters); }
	slice<overlayi> getoverlays() { return slice<overlayi>(overlays, state.overlays); }
	overlayi*		getoverlay(pointc v, directions d);
	overlayi*		getoverlay(pointc v, celln type);
	bool			is(pointc v, cellfn i) const;
	bool			is(pointc v, celln t1, celln t2) const;
	bool			isactive(const overlayi* p) const { return false; }
	void			makewave(pointc start) const;
	void			remove(pointc v, cellfn i);
	void			set(pointc v, celln i);
	void			set(pointc v, celln i, directions d);
	void			set(pointc v, celln i, pointc size);
	void			set(pointc v, cellfn i);
};
extern dungeoni *loc, *locup, *locdw;
extern unsigned short pathmap[mpy][mpx];

void dungeon_create();
bool filter_corridor(pointc v);
void show_automap(bool visible_all = false, bool secrets = false, const pointca* red_markers = 0);