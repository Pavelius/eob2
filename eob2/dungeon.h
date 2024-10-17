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
		explicit operator bool() const { return item::operator bool(); }
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
	struct overlayitem : item {
		unsigned short storage_index;
	};
	unsigned short	quest_id;
	dungeon_state	state;
	ground			items[512];
	creaturei		monsters[200];
	overlayi		overlays[255];
	overlayitem		overlayitems[256];
	unsigned char	data[mpy][mpx];
	overlayi*		add(pointc v, directions d, celln i);
	void			add(overlayi* po, item& it);
	void			addmonster(pointc v, directions d, int side, const monsteri* pi);
	int				around(pointc v, celln t1, celln t2) const;
	void			block(bool treat_door_as_passable) const;
	void			clear();
	void			change(celln s, celln n);
	void			drop(pointc v, item& it, int side);
	celln			get(pointc v) const;
	overlayi*		get(pointc v, directions d);
	size_t			getitems(ground** result, size_t result_maximum, pointc v);
	size_t			getitems(item** result, size_t result_maximum, const overlayi* po);
	void			getmonsters(creaturei** result, pointc index, directions dr);
	overlayi*		getoverlay(pointc v, celln type);
	bool			have(const overlayi* p) const { return p >= overlays && p <= overlays + lenghtof(overlays); }
	bool			is(pointc v, cellfn i) const;
	bool			is(pointc v, celln t1, celln t2) const;
	void			makewave(pointc start) const;
	void			remove(pointc v, cellfn i);
	void			removeov(pointc v);
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

int get_side(int side, directions d);
