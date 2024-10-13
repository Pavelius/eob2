#include "assign.h"
#include "answers.h"
#include "cell.h"
#include "dungeon.h"
#include "direction.h"
#include "math.h"
#include "pointca.h"
#include "rand.h"
#include "rect.h"
#include "resid.h"
#include "shape.h"
#include "wallmessage.h"

#ifdef _DEBUG
#define DEBUG_DUNGEON
//#define DEBUG_ROOM
#endif

typedef void (*fncorridor)(pointc v, directions d);
typedef void (*fnroom)(pointc v, directions d, const shapei* p);

const int EmpthyStartIndex = 1;

static directions all_directions[] = {Up, Down, Left, Right};
static posable rooms[256]; // Generation ring buffer
static unsigned char stack_put; // Stack top
static unsigned char stack_get; // Stack bottom

static void show_map_interactive() {
	show_automap(false);
}

static void select_pathable(pointca& result) {
	result.clear();
	auto ps = result.begin();
	auto pe = result.endof();
	pointc v;
	for(v.y = 0; v.y < mpy; v.y++) {
		for(v.x = 0; v.x < mpx; v.x++) {
			if(!pathmap[v.y][v.x] || pathmap[v.y][v.x] == 0xFFFF)
				continue;
			if(ps < pe)
				*ps++ = v;
		}
	}
	result.count = ps - result.data;
}

static void show_map_pathfind() {
	pointca points;
	loc->block(true);
	loc->makewave(loc->state.up);
	select_pathable(points);
	show_automap(false, true, &points);
}

static directions optimal_direction(pointc v) {
	directions d = Left;
	int i = v.x;
	if(i < (mpx - v.x)) {
		i = mpx - v.x;
		d = Right;
	}
	if(i < v.y) {
		i = v.y;
		d = Up;
	}
	if(i < mpy - v.y) {
		i = mpy - v.y;
		d = Down;
	}
	return d;
}

static void put_corridor(pointc v, directions d, unsigned flags, bool test_valid) {
	if(!v)
		return;
	if(test_valid && !loc->is(to(v, d), CellUnknown, CellUnknown))
		return;
	rooms[stack_put++] = posable(v, d);
}

static void setwall(pointc v, directions d) {
	loc->set(to(v, d), CellWall);
}

static bool isaround(pointc v, directions dir, celln t1 = CellUnknown) {
	if(!loc->is(to(v, to(dir, Left)), CellUnknown, t1))
		return false;
	if(!loc->is(to(v, to(dir, Right)), CellUnknown, t1))
		return false;
	if(!loc->is(to(v, to(dir, Up)), CellUnknown, t1))
		return false;
	return true;
}

static bool isboth(pointc v, directions d1, directions d2, celln t1, celln t2 = CellUnknown) {
	auto c1 = loc->get(to(v, d1));
	auto c2 = loc->get(to(v, d2));
	return (c1 == t1 || c1 == t2) && (c2 == t1 || c2 == t2);
}

static bool is(pointc v, directions d, celln t1) {
	return loc->get(to(v, d)) == t1;
}

static bool door(pointc v, directions d, bool has_button, bool has_button_on_other_side) {
	if(loc->type == FOREST)
		return true;
	switch(loc->get(v)) {
	case CellWall:
	case CellPortal:
		return false;
	}
	// If nearbe at least one door present, don't create new one.
	if(loc->around(v, CellDoor, CellDoor))
		return false;
	// If there is no walls from two sides, don't create door.
	if(!isboth(v, Up, Down, CellWall) && !isboth(v, Left, Right, CellWall))
		return false;
	loc->set(v, CellDoor);
	if(has_button)
		loc->add(to(v, to(d, Down)), CellDoorButton, d);
	if(has_button_on_other_side)
		loc->add(to(v, d), CellDoorButton, to(d, Down));
	return true;
}

static void items(pointc v, unsigned char type, int bonus_level) {
	// TODO: item creating
}

static unsigned char item_type(const char* id) {
	auto p = bsdata<itemi>::find(id);
	if(!p)
		return 0;
	return p - bsdata<itemi>::elements;
}

static unsigned char random_item_type() {
	// TODO: item type choose
	return 0;
}

static int random_count() {
	auto r = d100();
	if(r < 50)
		return 0;
	else if(r < 85)
		return 1;
	else
		return 2;
}

static void items(pointc v, int bonus_level) {
	items(v, random_item_type(), bonus_level);
}

static void secret(pointc v, directions d) {
	auto v1 = to(v, d);
	if(!v1)
		return;
	if(!loc->is(v1, CellWall, CellUnknown))
		return;
	if(!loc->is(to(v1, to(d, Left)), CellWall, CellUnknown))
		return;
	if(!loc->is(to(v1, to(d, Right)), CellWall, CellUnknown))
		return;
	auto v2 = to(v1, d);
	if(!v2)
		return;
	if(loc->around(v2, CellWall, CellUnknown) != 4)
		return;
	loc->set(v1, CellWall);
	loc->add(v, CellSecrectButton, d);
	loc->set(v2, CellPassable);
	int count = 1;
	if(d100() < 25)
		count = 2;
	for(int i = 0; i < count; i++)
		items(v2, 3);
	loc->set(to(v2, to(d, Left)), CellWall);
	loc->set(to(v2, to(d, Right)), CellWall);
	loc->set(to(v2, to(d, Up)), CellWall);
	loc->state.secrets++;
}

static void monster(pointc v, directions d) {
	auto n = (d100() < 30) ? 1 : 0;
	loc->addmonster(v, loc->habbits[n]);
}

static void prison(pointc v, directions d) {
	if(loc->type == FOREST)
		return;
	auto v1 = to(v, d);
	if(!loc->is(v1, CellWall, CellUnknown))
		return;
	if(!loc->is(to(v1, to(d, Left)), CellWall, CellUnknown))
		return;
	if(!loc->is(to(v1, to(d, Right)), CellWall, CellUnknown))
		return;
	auto v2 = to(v1, d);
	if(!isaround(v2, d, CellWall))
		return;
	loc->set(v1, CellDoor);
	loc->add(v, CellDoorButton, d);
	loc->set(to(v1, to(d, Left)), CellWall);
	loc->set(to(v1, to(d, Right)), CellWall);
	loc->set(v2, CellPassable);
	for(int i = random_count(); i > 0; i--)
		items(v2, 0);
	monster(v2, Down);
	loc->set(to(v2, to(d, Left)), CellWall);
	loc->set(to(v2, to(d, Right)), CellWall);
	loc->set(to(v2, to(d, Up)), CellWall);
}

static void treasure(pointc v, directions d) {
	if(loc->type == FOREST)
		return;
	auto v1 = to(v, d);
	if(!loc->is(v1, CellWall, CellUnknown))
		return;
	if(!loc->is(to(v1, to(d, Left)), CellWall, CellUnknown))
		return;
	if(!loc->is(to(v1, to(d, Right)), CellWall, CellUnknown))
		return;
	if(!loc->is(to(v, to(d, Right)), CellPassable, CellUnknown))
		return;
	auto v2 = to(v1, d);
	if(!isaround(v2, d, CellWall))
		return;
	auto magic_bonus = 2;
	loc->set(v1, CellDoor);
	loc->add(to(v, to(d, Right)), CellKeyHole, d);
	loc->set(to(v1, to(d, Left)), CellWall);
	loc->set(to(v1, to(d, Right)), CellWall);
	loc->set(v2, CellPassable);
	for(auto i = 1 + random_count(); i > 0; i--)
		items(v2, magic_bonus);
	loc->set(to(v2, to(d, Left)), CellWall);
	loc->set(to(v2, to(d, Right)), CellWall);
	loc->set(to(v2, to(d, Up)), CellWall);
}

static void decoration(pointc v, directions d) {
	if(loc->type == FOREST)
		return;
	auto v1 = to(v, d);
	if(!loc->is(v1, CellWall, CellUnknown))
		return;
	static celln random[] = {CellDecor1, CellDecor2, CellDecor3};
	loc->set(v1, CellWall);
	loc->add(v, maprnd(random), d);
}

static void portal(pointc v, directions d) {
	if(loc->type == FOREST)
		return;
	if(loc->state.portal)
		return;
	auto v1 = to(v, d);
	if(!loc->is(v1, CellWall, CellUnknown))
		return;
	if(!loc->is(to(v1, to(d, Left)), CellWall, CellUnknown))
		return;
	if(!loc->is(to(v1, to(d, Right)), CellWall, CellUnknown))
		return;
	auto v2 = to(v1, d);
	if(!loc->is(v2, CellWall, CellUnknown))
		return;
	loc->set(v1, CellPortal);
	loc->set(v2, CellWall);
}

static void message(pointc v, directions d) {
	if(loc->type == FOREST)
		return;
	if(loc->state.messages > MessageHabbits)
		return;
	auto v1 = to(v, d);
	if(!loc->is(v1, CellWall, CellUnknown))
		return;
	loc->set(v1, CellWall);
	auto po = loc->add(v, CellMessage, d);
	po->subtype = (unsigned char)loc->state.messages;
	loc->state.messages++;
}

static pointc find_free_wall(pointc v, directions d) {
	while(true) {
		auto v1 = to(v, d);
		if(!v1)
			return {-1, -1};
		switch(loc->get(v1)) {
		case CellWall:
			if(loc->getoverlay(v, d))
				return {-1, -1};
			return v;
		case CellPassable:
		case CellButton:
		case CellPit:
			break;
		default:
			return {-1, -1};
		}
		v = v1;
	}
}

static void trap(pointc v, directions d) {
	if(loc->type == FOREST)
		return;
	if(!v)
		return;
	directions all_directions[] = {Down, Left, Right, Up};
	pointc trap_launch;
	for(auto dr : all_directions) {
		auto d1 = to(d, dr);
		trap_launch = find_free_wall(v, d1);
		if(trap_launch) {
			d = d1;
			break;
		}
	}
	if(!trap_launch)
		return;
	loc->set(v, CellButton);
	auto po = loc->add(trap_launch, CellTrapLauncher, d);
	po->link = v;
	loc->state.traps++;
}

static int random_cellar_count() {
	auto rolled = d100();
	if(rolled < 60)
		return 0;
	else if(rolled < 90)
		return 1;
	return 2;
}

static void cellar(pointc v, directions d) {
	if(loc->type == FOREST)
		return;
	auto v1 = to(v, d);
	if(!loc->is(v1, CellWall, CellUnknown))
		return;
	loc->set(v1, CellWall);
	auto po = loc->add(v, CellCellar, d);
	auto count = random_cellar_count();
	while(count > 0) {
		//auto i1 = create_item(pd, random_type(true), 2);
		// Items in cellar can be identified
		//if(d100() < 60)
		//	i1.setidentified(1);
		//loc->add(po, i1);
		count--;
	}
}

static void empthy(pointc v, directions d) {}

static void rations(pointc v, directions d) {
	items(v, item_type("Ration"), 0);
}

static void stones(pointc v, directions d) {
	items(v, item_type("Stone"), 0);
}

static bool ispassable(pointc v, directions d) {
	v = to(v, d);
	if(!v)
		return false;
	return loc->is(v, CellPassable, CellUnknown);
}

static bool corridor(pointc v, directions d) {
	auto chance = 0;
	if(!v)
		return false;
	directions rnd[] = {Right, Left};
	if(d100() < 50)
		iswap(rnd[0], rnd[1]);
	pointc start;
	while(true) {
		auto v1 = to(v, d);
		if(!v1 || loc->get(v1) != CellUnknown)
			break;
		if(!start)
			start = v;
		v = v1;
		loc->set(v, CellPassable);
		if(d100() < chance || !to(v, d))
			break;
		// Wall to the left only if there is no other tiles
		if(loc->get(to(v, to(d, Left))) == CellUnknown)
			setwall(v, to(d, Left));
		// Wall to the right only if there is no other tiles
		if(loc->get(to(v, to(d, Right))) == CellUnknown)
			setwall(v, to(d, Right));
		auto random_content = true;
		if((chance == 0) && d100() < 30) {
			if(door(v, d, true, true))
				random_content = false;
		}
		if(random_content) {
			static fncorridor corridor_random[] = {empthy,
				empthy, empthy, empthy, empthy, empthy, empthy,
				empthy, empthy, empthy, empthy,
				secret,
				monster, monster, monster, monster,
				rations,
				stones,
				trap,
				cellar,
				portal,
				prison, prison,
				treasure,
				decoration, decoration,
				message,
			};
			auto proc = maprnd(corridor_random);
			proc(v, to(d, rnd[0]));
			if(d100() < 60)
				iswap(rnd[0], rnd[1]);
		}
		chance += 13;
	}
	if(!start)
		return false;
	auto passes = 0;
	if(ispassable(v, to(d, rnd[0]))) {
		passes++;
		put_corridor(v, to(d, rnd[0]), 0, false);
	}
	if(ispassable(v, to(d, rnd[1]))) {
		passes++;
		put_corridor(v, to(d, rnd[1]), 0, false);
	}
	if(ispassable(v, d)) {
		if(passes < 1)
			put_corridor(v, d, 0, false);
	}
	return true;
}

static bool random_corridor(pointc v) {
	directions dir[] = {Up, Down, Left, Right};
	zshuffle(dir, sizeof(dir) / sizeof(dir[0]));
	for(auto d : dir) {
		if(corridor(v, d))
			return true;
	}
	return false;
}

static void remove_all_overlay(pointc v) {
	if(!v)
		return;
	for(auto& e : loc->getoverlays()) {
		if(e.d == Center)
			continue;
		if(to(e, e.d) == v)
			e.clear();
	}
}

static void drop_special(pointc v, item& it) {
	//location.dropitem(index, it, 0);
	loc->state.special = v;
	it.clear();
}

static void remove_dead_door() {
	pointc v;
	for(v.y = 0; v.y < mpy; v.y++) {
		for(v.x = 0; v.x < mpx; v.x++) {
			if(loc->get(v) != CellDoor)
				continue;
			// Door correct if place between two walls horizontally
			if(isboth(v, Left, Right, CellWall, CellWall)
				|| isboth(v, Up, Down, CellWall, CellWall))
				continue;
			// Door correct if there is exacly 2 walls around
			if(loc->around(v, CellWall, CellWall) == 2)
				continue;
			// Incorrect door must be eliminated
			loc->set(v, CellPassable);
			remove_all_overlay(v);
		}
	}
}

static int total_levels(slice<dungeon_site> source) {
	auto result = 0;
	for(auto& e : source)
		result += e.level;
	return result;
}

static bool is_valid_dungeon() {
	if(!loc->state.down || !loc->state.up)
		return true;
	loc->block(true);
	loc->makewave(loc->state.up);
	if(!pathmap[loc->state.down.y][loc->state.down.x])
		return false;
	if(loc->state.lair && !pathmap[loc->state.lair.y][loc->state.lair.x])
		return false;
	if(loc->state.feature && !pathmap[loc->state.feature.y][loc->state.feature.x])
		return false;
	return true;
}

static void create_points(pointca& points, int mx, int my, int offset) {
	points.clear();
	for(auto x = 0; x < mx; x++) {
		for(auto y = 0; y < my; y++) {
			rect rc;
			rc.x1 = x * mpx / mx; rc.x2 = (x + 1) * mpx / mx;
			rc.y1 = y * mpy / my; rc.y2 = (y + 1) * mpy / my;
			rc.offset(offset);
			if(rc.x1 >= rc.x2 || rc.y1 >= rc.y2)
				continue;
			pointc v;
			v.x = rc.x1 + rand() % rc.width();
			v.y = rc.y1 + rand() % rc.height();
			points.add(v);
		}
	}
	zshuffle(points.data, points.count);
}

static pointc pop(pointca& points) {
	auto n = points[0];
	points.remove(0);
	return n;
}

static void apply_shape(pointc v, directions d, const shapei* shape, char sym, celln t) {
	pointc c;
	if(!shape)
		return;
	for(c.y = 0; c.y < shape->size.y; c.y++) {
		for(c.x = 0; c.x < shape->size.x; c.x++) {
			auto n = (*shape)[c];
			if(n == sym)
				loc->set(shape->translate(v, c, d), t);
		}
	}
}

static void apply_shape(pointc v, directions d, const shapei* shape, char sym, fncorridor proc) {
	pointc c;
	if(!shape)
		return;
	for(c.y = 0; c.y < shape->size.y; c.y++) {
		for(c.x = 0; c.x < shape->size.x; c.x++) {
			auto n = (*shape)[c];
			if(n == sym)
				proc(shape->translate(v, c, d), d);
		}
	}
}

static void stairs_up(pointc v, directions d, const shapei* ps) {
	apply_shape(v, d, ps, '0', CellStairsUp);
	apply_shape(v, d, ps, '1', CellPassable);
}

static void stairs_down(pointc v, directions d, const shapei* ps) {
	apply_shape(v, d, ps, '0', CellStairsDown);
	apply_shape(v, d, ps, '1', CellPassable);
}

static void create_lair(pointc v, directions d, const shapei* ps) {
	apply_shape(v, d, ps, '0', CellPassable);
	apply_shape(v, d, ps, '1', CellDoor);
	apply_shape(v, d, ps, '.', monster);
}

static void create_room(pointc v, directions d, const char* id, fnroom proc) {
	if(!v)
		return;
	auto ps = bsdata<shapei>::find(id);
	if(!ps)
		return;
	apply_shape(v, d, ps, 'X', CellWall);
	apply_shape(v, d, ps, '.', CellPassable);
	proc(v, d, ps);
	put_corridor(ps->translate(v, ps->points[1], d), d, EmpthyStartIndex, false);
#ifdef DEBUG_ROOM
	show_map_interactive();
#endif
}

static void create_room(pointc v, const char* id, fnroom proc) {
	create_room(v, optimal_direction(v), id, proc);
}

static void create_rooms(pointc start, bool last_level) {
	pointca points;
	create_points(points, 3, 2, 3);
	if(start)
		create_room(start, "ShapeExit", stairs_up);
	else
		create_room(pop(points), "ShapeExit", stairs_up);
	if(!last_level)
		create_room(pop(points), "ShapeExit", stairs_down);
	// Every dungeon have one lair where monsters spawn
	loc->state.lair = pop(points);
	create_room(loc->state.lair, "ShapeRoom", create_lair);
	// And every level have feature
	loc->state.feature = pop(points);
	create_room(loc->state.feature, "ShapeLargeRoom", create_lair);
}

static void dungeon_create(slice<dungeon_site> source) {
	auto base = 0;
	auto total_level_count = total_levels(source);
	dungeoni* previous = 0;
	for(auto& ei : source) {
		auto special_item_level = -1;
		if(ei.special)
			special_item_level = rand() % ei.level;
		for(auto j = 0; j < ei.level; j++) {
			loc = bsdata<dungeoni>::add();
			auto level = base + j + 1;
			posable start;
			if(previous)
				start = previous->state.down;
			auto last_level = (level == total_level_count);
			while(true) {
				loc->clear();
				assign<dungeon_site>(*loc, ei);
				loc->level = level;
				loc->cursed = 5;
				if(special_item_level != j)
					loc->special = 0;
				create_rooms(start, last_level);
				while(stack_get != stack_put) {
					auto& ev = rooms[stack_get++];
					auto result = corridor(ev, ev.d);
					if(!result)
						random_corridor(ev);
					loc->state.elements++;
#ifdef DEBUG_ROOM
					show_map_interactive();
#endif
				}
				loc->change(CellUnknown, CellWall);
				if(is_valid_dungeon())
					break;
			}
			remove_dead_door();
#ifdef DEBUG_DUNGEON
			show_map_pathfind();
#endif
			//if(j == special_item_level)
			//	validate_special_items(e);
			//add_spawn_points(e);
			//add_corners(e, p->crypt.corner, p->crypt.corner_count);
			previous = loc;
		}
		base += ei.level;
	}
	// Add dungeon pits and other stuff
	//for(unsigned i = 0; i < count - 1; i++)
	//	link_dungeon(dungeons[i], dungeons[i + 1]);
}

void dungeon_create() {
	static dungeon_site source[] = {
		{DUNG, 3},
		{DUNG, 2},
	};
	dungeon_create(source);
}