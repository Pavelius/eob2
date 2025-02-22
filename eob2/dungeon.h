#pragma once

#include "creature.h"
#include "dungeon_site.h"
#include "dungeon_state.h"
#include "goal.h"
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
		void		set(cellfn v) { flags.set(v); }
	};
	struct overlayitem : item {
		unsigned short storage_index;
	};
	unsigned short	quest_id;
	dungeon_state	state;
	ground			items[512];
	creaturei		monsters[256];
	overlayi		overlays[256];
	overlayitem		overlayitems[256];
	celln			data[mpy][mpx];
	unsigned char	flags[mpy][mpx];
	goalf			rewards;
	overlayi*		add(pointc v, directions d, celln i);
	void			add(overlayi* po, item& it);
	void			addmonster(pointc v, directions d, int side, const monsteri* pi);
	int				around(pointc v, celln t1, celln t2) const;
	void			block(bool treat_door_as_passable) const;
	void			broke(pointc v);
	void			clear();
	void			change(celln s, celln n);
	void			drop(pointc v, item& it, int side);
	celln			get(pointc v) const;
	overlayi*		get(pointc v, directions d);
	overlayi*		getlinked(pointc v);
	directions		getnear(pointc v, celln c) const;
	size_t			getitems(ground** result, size_t result_maximum, pointc v);
	size_t			getitems(item** result, size_t result_maximum, const overlayi* po);
	itemi*			getkey() const;
	creaturei*		getmonster(short unsigned monster_id);
	void			getmonsters(creaturei** result, pointc index, directions dr);
	void			getmonsters(creaturei** result, pointc index);
	overlayi*		getoverlay(pointc v, celln type);
	void			getoverlays(pointca& result, celln type, bool hidden) const;
	directions		getpassable(pointc v) const;
	int				getpassables(bool explored) const;
	bool			have(const overlayi* p) const { return p >= overlays && p <= overlays + lenghtof(overlays); }
	bool			have(const creaturei* p) const { return p >= monsters && p <= monsters + lenghtof(monsters); }
	bool			is(pointc v, cellfn i) const;
	bool			is(pointc v, celln t1, celln t2) const;
	bool			is(goaln v) const { return rewards.is(v); }
	bool			is(fnpointc v) const;
	bool			isitem(pointc v) const;
	bool			isforbidden(pointc v) const;
	bool			ismonster(pointc v) const;
	bool			ismonster(pointc v, featn f) const;
	bool			isoverlay(pointc v) const;
	bool			isoverlay(pointc v, directions d) const { return const_cast<dungeoni*>(this)->get(v, d) != 0; }
	bool			ispassable(pointc v) const;
	static void		makewave(pointc start);
	void			markoverlay(celln type, short unsigned value) const;
	void			remove(pointc v, cellfn i);
	void			removeov(pointc v);
	void			set(pointc v, celln i);
	void			set(pointc v, celln i, directions d);
	void			set(pointc v, celln i, pointc size);
	void			set(pointc v, cellfn i, int radius);
	void			set(pointc v, cellfn i);
};
extern dungeoni *loc, *locup, *last_dungeon;

const char* get_part_placement(pointc v);

void dungeon_create();
void enter_active_dungeon();
bool filter_corridor(pointc v);
void show_automap(bool show_fog_of_war, bool show_secrets, bool show_party, const pointca* red_markers);
void show_automap(const pointca& markers, int explore_radius);
void show_dungeon_automap();

int get_side(int side, directions d);
int get_side_ex(int side, directions d);

void thrown_item(pointc v, directions d, int avatar_thrown, int side, int distance);