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

typedef void (*fnroom)(pointc v, directions d, const shapei* p);

const int EmpthyStartIndex = 1;

static directions all_directions[] = {Up, Down, Left, Right};
static posable rooms[256]; // Generation ring buffer
static unsigned char stack_put; // Stack top
static unsigned char stack_get; // Stack bottom

static int chance_cursed = 5;

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

static bool door(pointc v, directions d, bool has_button, bool has_button_on_other_side) {
	if(loc->type == FOREST)
		return true;
	auto v1 = to(v, d);
	switch(loc->get(v1)) {
	case CellWall:
	case CellPortal:
		return false;
	}
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

static void secret(pointc v, directions d, unsigned flags) {
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

static void monster(pointc v, directions d, unsigned flags) {
	auto n = (d100() < 30) ? 1 : 0;
	loc->addmonster(v, loc->habbits[n]);
}

static void prison(pointc v, directions d, unsigned flags) {
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
	monster(v2, Down, 0);
	loc->set(to(v2, to(d, Left)), CellWall);
	loc->set(to(v2, to(d, Right)), CellWall);
	loc->set(to(v2, to(d, Up)), CellWall);
}

static void treasure(pointc v, directions d, unsigned flags) {
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

static void decoration(pointc v, directions d, unsigned flags) {
	if(loc->type == FOREST)
		return;
	auto v1 = to(v, d);
	if(!loc->is(v1, CellWall, CellUnknown))
		return;
	static celln random[] = {CellDecor1, CellDecor2, CellDecor3};
	loc->set(v1, CellWall);
	loc->add(v, maprnd(random), d);
}

static void portal(pointc v, directions d, unsigned flags) {
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
	if(!loc->is(to(v1, d), CellWall, CellUnknown))
		return;
	loc->set(to(v1, to(d, Down)), CellPortal);
}

static void message(pointc v, directions d, unsigned flags) {
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
			return v1;
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

static void trap(pointc v, directions d, unsigned flags) {
	if(loc->type == FOREST)
		return;
	auto dr = to(d, Left);
	auto v1 = find_free_wall(v, d);
	if(!v1) {
		d = to(d, Right);
		v1 = find_free_wall(v, d);
		if(!v1)
			return;
	}
	loc->set(v, CellButton);
	auto po = loc->add(v1, CellTrapLauncher, dr);
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

static void cellar(pointc v, directions d, unsigned flags) {
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

static void empthy(pointc v, directions d, unsigned flags) {}

static void rations(pointc v, directions d, unsigned flags) {
	items(v, item_type("Ration"), 0);
}

static void stones(pointc v, directions d, unsigned flags) {
	items(v, item_type("Stone"), 0);
}

static bool corridor(pointc v, directions d, unsigned flags) {
	auto chance = 0;
	if(!v)
		return false;
	directions rnd[] = {Right, Left};
	if(d100() < 50)
		iswap(rnd[0], rnd[1]);
	pointc start;
	while(true) {
		auto new_index = to(v, d);
		if(!new_index || loc->get(new_index) != CellUnknown)
			break;
		bool random_content = true;
		if(!start) {
			start = v;
			if(flags & EmpthyStartIndex)
				random_content = false;
		}
		v = new_index;
		loc->set(v, CellPassable);
		if(d100() < chance || !to(v, d))
			break;
		setwall(v, to(d, Left));
		setwall(v, to(d, Right));
		if(random_content && (chance == 0) && d100() < 30) {
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
			auto proc = corridor_random[rand() % (sizeof(corridor_random) / sizeof(corridor_random[0]))];
			proc(v, to(d, rnd[0]), flags);
			if(d100() < 60)
				iswap(rnd[0], rnd[1]);
		}
		chance += 12;
	}
	if(!start)
		return false;
	//auto passes = 0;
	//if(ispassable(pd, to(index, to(dir, rnd[0])))) {
	//	passes++;
	//	putroom(pd, index, to(dir, rnd[0]), 0, false);
	//}
	//if(ispassable(pd, to(index, to(dir, rnd[1])))) {
	//	passes++;
	//	putroom(pd, index, to(dir, rnd[1]), 0, false);
	//}
	//if(ispassable(pd, to(index, dir))) {
	//	if(passes < 1)
	//		putroom(pd, index, dir, 0, false);
	//}
	return true;
}

static bool random_corridor(pointc v, unsigned flags) {
	directions dir[] = {Up, Down, Left, Right};
	zshuffle(dir, sizeof(dir) / sizeof(dir[0]));
	for(auto d : dir) {
		if(corridor(v, d, flags))
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
			auto t = loc->get(v);
			if(t != CellDoor)
				continue;
			// Door correct if place between two walls horizontally
			if(loc->get(to(v, Left)) == CellWall && loc->get(to(v, Right)) == CellWall)
				continue;
			// Door correct if place between two walls vertically
			if(loc->get(to(v, Up)) == CellWall && loc->get(to(v, Down)) == CellWall)
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
	return pathmap[loc->state.down.y][loc->state.down.x] != 0;
}

static void create_points(int mx, int my, int offset) {
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

static pointc pop_point() {
	auto n = points[0];
	points.remove(0);
	return n;
}

static void apply_shape(pointc v, directions d, const shapei* shape, char sym, celln t) {
	pointc c;
	if(!shape)
		return;
	for(c.y = 0; c.y < shape->size.y; c.y) {
		for(c.x = 0; c.x < shape->size.x; c.x) {
			auto n = (*shape)[c];
			if(n == sym)
				loc->set(shape->translate(c, v, d), t);
		}
	}
}

static void stairs_up(pointc v, directions d, const shapei* ps) {
	apply_shape(v, d, ps, '0', CellStairsUp);
}

static void stairs_down(pointc v, directions d, const shapei* ps) {
	apply_shape(v, d, ps, '0', CellStairsDown);
}

static void create_room(pointc v, directions d, const char* id, fnroom proc) {
	if(!v)
		return;
	auto ps = bsdata<shapei>::find(id);
	if(!ps)
		return;
	apply_shape(v, d, ps, 'X', CellPassable);
	apply_shape(v, d, ps, '.', CellPassable);
	proc(v, d, ps);
	put_corridor(ps->translate(ps->points[1], v, d), d, EmpthyStartIndex, false);
}

static void create_room(unsigned char place, fncorridor proc) {
//	indext indecies[10]; point size;
//	auto dir = maprnd(all_around);
//	e.set(0, dir, place, size, indecies, false);
//	auto m = imax(size.x, size.y) + 2;
//	short x = xrand(m, mpx - m - 1), y = xrand(m, mpy - m - 1);
//	auto i = e.getvalid(e.getindex(x, y), size.x, size.y, CellUnknown);
//	create_room(e, i, place, dir, site, proc);
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
				if(special_item_level != j)
					loc->special = 0;
				loc->level = level;
				create_points(3, 2, 2);
				// e.chance.curse = 5 + p->chance.curse;
				if(start)
					create_room(start, to(start.d, Down), "ShapeExit", stairs_up);
				else
					create_room(pop_point(), maprnd(all_directions), "ShapeExit", stairs_up);
				if(!last_level)
					create_room(pop_point(), maprnd(all_directions), "ShapeExit", stairs_down);
				// Every dungeon have one lair
				//create_room(e, ShapeRoom, p, create_lair);
				while(stack_get != stack_put) {
					auto& ev = rooms[stack_get++];
					auto result = corridor(ev, ev.d, ev.side);
					if(!result)
						random_corridor(ev, ev.side);
					loc->state.elements++;
					if(show_interactive)
						show_automap(true);
				}
				loc->change(CellUnknown, CellWall);
				if(is_valid_dungeon())
					break;
				break;
			}
			remove_dead_door();
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
}