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
		flag8		flags;
		void		clear();
		bool		is(cellfn v) const { return flags.is(v); }
		void		remove() { clear(); }
	};
	dungeon_state	state;
	unsigned short	quest_id;
	ground			items[512];
	creaturei		monsters[200];
	overlayi		overlays[255];
	unsigned char	data[mpy][mpx];
	overlayi*		add(pointc v, directions d, celln i);
	void			addmonster(pointc v, unsigned char type) {}
	int				around(pointc v, celln t1, celln t2) const;
	void			block(bool treat_door_as_passable) const;
	void			clear();
	void			change(celln s, celln n);
	celln			get(pointc v) const;
	overlayi*		get(pointc v, directions d);
	slice<ground>	getitems() { return slice<ground>(items, state.items); }
	slice<creaturei> getmonsters() { return slice<creaturei>(monsters, state.monsters); }
	slice<overlayi> getoverlays() { return slice<overlayi>(overlays, state.overlays); }
	overlayi*		getoverlay(pointc v, celln type);
	bool			is(pointc v, cellfn i) const;
	bool			is(pointc v, celln t1, celln t2) const;
	void			makewave(pointc start) const;
	void			remove(pointc v, cellfn i);
	void			set(pointc v, celln i);
	void			set(pointc v, celln i, directions d);
	void			set(pointc v, celln i, pointc size);
	void			set(pointc v, cellfn i);
};
extern dungeoni *loc, *locup, *locdw;
extern unsigned short pathmap[mpy][mpx];

const char* get_part_placement(pointc v);

void dungeon_create();
bool filter_corridor(pointc v);
void show_automap(bool show_fog_of_war, bool show_secrets, bool show_party, const pointca* red_markers);
void show_dungeon_automap();
