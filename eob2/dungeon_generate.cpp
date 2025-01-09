#include "assign.h"
#include "answers.h"
#include "cell.h"
#include "dungeon.h"
#include "list.h"
#include "math.h"
#include "monster.h"
#include "pointca.h"
#include "quest.h"
#include "rand.h"
#include "randomizer.h"
#include "rect.h"
#include "resid.h"
#include "room.h"
#include "shape.h"
#include "wallmessage.h"

#ifdef _DEBUG
// #define DEBUG_DUNGEON
// #define DEBUG_ROOM
#endif

static directions all_directions[] = {Up, Down, Left, Right};
static posable rooms[256]; // Generation ring buffer
static unsigned char stack_put; // Stack top
static unsigned char stack_get; // Stack bottom

#ifdef DEBUG_ROOM
static void show_map_interactive() {
	show_automap(false, true, false, 0);
}
#endif

#ifdef DEBUG_DUNGEON
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
static void show_map_features() {
	pointca points;
	points.add(loc->state.up);
	if(loc->state.down)
		points.add(loc->state.down);
	for(auto v : loc->state.features)
		points.add(v);
	show_automap(false, true, false, &points);
}
static void show_map_pathfind() {
	pointca points;
	loc->block(true);
	loc->makewave(loc->state.up);
	select_pathable(points);
	show_automap(false, true, false, 0);// &points);
}
#endif

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

static int random_count() {
	auto rolled = d100();
	if(rolled < 70)
		return 0;
	else if(rolled < 95)
		return 1;
	return 2;
}

static int cellar_count() {
	auto rolled = d100();
	if(rolled < 40)
		return 0;
	else if(rolled < 70)
		return 1;
	else if(rolled < 95)
		return 2;
	return 3;
}

static void put_corridor(pointc v, directions d, bool test_valid) {
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
	switch(loc->get(v)) {
	case CellWall:
	case CellPortal:
		return false;
	default: break;
	}
	if(loc->type == FOREST) {
		loc->set(v, CellPassable);
		return true;
	}
	// If nearbe at least one door present, don't create new one.
	if(loc->around(v, CellDoor, CellDoor))
		return false;
	// If there is no walls from two sides, don't create door.
	if(!isboth(v, Up, Down, CellWall) && !isboth(v, Left, Right, CellWall))
		return false;
	loc->set(v, CellDoor);
	if(has_button)
		loc->add(to(v, to(d, Down)), d, CellDoorButton);
	if(has_button_on_other_side)
		loc->add(to(v, d), to(d, Down), CellDoorButton);
	return true;
}

static void lair_door(pointc v, directions d) {
	door(v, d, true, true);
}

static int get_magic_bonus(int chance_upgrade, int chance_downgrade) {
	auto base = loc->level;
	if(last_quest)
		base += last_quest->difficult;
	if(base < 1)
		base = 1;
	while(base < 5 && d100() < chance_upgrade) {
		if(base > 1 && d100() < chance_downgrade)
			base--;
		else
			base++;
	}
	if(base > 5)
		base = 5;
	return base;
}

static void create_item(item& it, itemi* pi, int bonus_level = 0, int chance_identify = 0) {
	// Any generated key match to dungeon key
	if(pi->wear == Key)
		pi = loc->getkey();
	it.create(pi - bsdata<itemi>::elements);
	auto chance_magic = (iabs(loc->level) + bonus_level) * 5 + loc->magical;
	auto chance_cursed = 5 + loc->cursed;
	auto magic_bonus = get_magic_bonus(20, 30);
	chance_identify += pi->chance_identify;
	it.createpower(magic_bonus, chance_magic, chance_cursed);
	if(it.isartifact())
		loc->state.wallmessages[MessageAtifacts]++;
	if(it.iscursed())
		loc->state.wallmessages[MessageCursedItems]++;
	if(chance_identify && d100() < chance_identify)
		it.identify(1);
	switch(pi->wear) {
	case Edible:
		// Food can be rotten
		if(d100() < 60)
			it.damage(5);
		break;
	case LeftRing: case RightRing:
		if(it.ismagical())
			loc->state.wallmessages[MessageMagicRings]++;
		break;
	case LeftHand: case RightHand:
		if(it.ismagical())
			loc->state.wallmessages[MessageMagicWeapons]++;
		break;
	}
}

static void items(pointc v, itemi* pi, int bonus_level = 0) {
	// TODO: item power generate
	if(!pi)
		return;
	item it;
	create_item(it, pi, bonus_level);
	loc->drop(v, it, xrand(0, 3));
}

static void items(pointc v, int bonus_level) {
	items(v, single("RandomItem"), bonus_level);
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
	loc->add(v, d, CellSecretButton);
	loc->set(v2, CellPassable);
	for(auto i = random_count() + 1; i > 0; i--)
		items(v2, 2);
	loc->set(to(v2, to(d, Left)), CellWall);
	loc->set(to(v2, to(d, Right)), CellWall);
	loc->set(to(v2, to(d, Up)), CellWall);
	loc->state.wallmessages[MessageSecrets]++;
}

static void monster(pointc v, directions d, const monsteri* pi, int count) {
	loc->set(v, CellPassable);
	if(pi->is(Large))
		loc->addmonster(v, d, 0, pi);
	else {
		int sides[4] = {0, 1, 2, 3};
		zshuffle(sides, 4);
		for(auto i = 0; i < count; i++)
			loc->addmonster(v, d, sides[i], pi);
	}
}

static void monster(pointc v, directions d) {
	auto n = (d100() < 30) ? 1 : 0;
	monster(v, d, bsdata<monsteri>::elements + loc->habbits[n], xrand(1, 4));
}

static void monster_boss(pointc v, directions d) {
	loc->set(v, CellPassable);
	auto pi = bsdata<monsteri>::elements + (loc->boss ? loc->boss : loc->habbits[1]);
	loc->addmonster(v, d, 0, pi);
}

static void monster_minion(pointc v, directions d) {
	auto pi = bsdata<monsteri>::elements + (loc->minions ? loc->minions : loc->habbits[1]);
	monster(v, d, pi, xrand(1, 4));
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
	loc->add(v, d, CellDoorButton);
	loc->set(to(v1, to(d, Left)), CellWall);
	loc->set(to(v1, to(d, Right)), CellWall);
	loc->set(v2, CellPassable);
	for(auto i = random_count(); i > 0; i--)
		items(v2, 0);
	monster(v2, to(d, Down));
	loc->set(to(v2, to(d, Left)), CellWall);
	loc->set(to(v2, to(d, Right)), CellWall);
	loc->set(to(v2, to(d, Up)), CellWall);
}

static void floor_treasure(pointc v, directions d) {
	loc->set(v, CellPassable);
	for(auto i = 3 + random_count(); i > 0; i--)
		items(v, 2);
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
	loc->set(v1, CellDoor);
	auto po = loc->add(to(v, to(d, Right)), d, CellKeyHole);
	if(po)
		po->link = v1;
	loc->state.wallmessages[MessageLocked]++;
	loc->set(to(v1, to(d, Left)), CellWall);
	loc->set(to(v1, to(d, Right)), CellWall);
	loc->set(v2, CellPassable);
	for(auto i = 1 + random_count(); i > 0; i--)
		items(v2, 1);
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
	if(!loc->get(v, d))
		loc->add(v, d, maprnd(random));
}

static void corridor_passable(pointc v, directions d) {
	loc->set(v, CellPassable);
}

static void corridor_stairs_down(pointc v, directions d) {
	loc->set(v, CellStairsDown);
	loc->state.down = v;
	loc->state.down.d = d;
}

static void corridor_stairs_up(pointc v, directions d) {
	loc->set(v, CellStairsUp);
	loc->state.up = v;
	loc->state.up.d = d;
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
	loc->set(to(v1, to(d, Left)), CellWall);
	loc->set(to(v1, to(d, Right)), CellWall);
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
	auto po = loc->add(v, d, CellMessage);
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
			if(loc->get(v, d))
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
	loc->set(v, CellButton);
}

static void resolve_traps() {
	pointc v;
	for(v.y = 0; v.y < mpy; v.y++) {
		for(v.x = 0; v.x < mpx; v.x++) {
			if(loc->get(v) != CellButton)
				continue;
			if(loc->around(v, CellWall, CellWall) >= 3) {
				loc->set(v, CellPassable);
				continue;
			}
			pointc trap_launch;
			directions launch_direction = Center;
			auto range = -1;
			for(auto d : all_directions) {
				auto tv = find_free_wall(v, d);
				if(!tv)
					continue;
				auto r = tv.distance(v);
				if(r <= range)
					continue;
				trap_launch = tv;
				range = r;
				launch_direction = d;
			}
			auto po = loc->add(trap_launch, launch_direction, CellTrapLauncher);
			if(po)
				po->link = v;
			loc->state.wallmessages[MessageTraps]++;
		}
	}
}

static void cellar(pointc v, directions d) {
	static variant random_list("RandomSmallItem");
	if(loc->type == FOREST)
		return;
	auto v1 = to(v, d);
	if(!loc->is(v1, CellWall, CellUnknown))
		return;
	loc->set(v1, CellWall);
	auto po = loc->add(v, d, CellCellar);
	auto count = cellar_count();
	while(count-- > 0) {
		item it;
		create_item(it, single(random_list), 0, 40);
		loc->add(po, it);
	}
}

static void empthy(pointc v, directions d) {}

static void rations(pointc v, directions d) {
	items(v, single("RandomRation"), 0);
}

static void stones(pointc v, directions d) {
	items(v, single("RandomStone"), 0);
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
			static fncorridor corridor_random[] = {
				empthy, empthy, empthy, empthy, empthy,
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
				decoration, decoration, decoration,
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
		put_corridor(v, to(d, rnd[0]), false);
	}
	if(ispassable(v, to(d, rnd[1]))) {
		passes++;
		put_corridor(v, to(d, rnd[1]), false);
	}
	if(ispassable(v, d)) {
		if(passes < 1)
			put_corridor(v, d, false);
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

static void remove_dead_door() {
	pointc v;
	for(v.y = 0; v.y < mpy; v.y++) {
		for(v.x = 0; v.x < mpx; v.x++) {
			if(loc->get(v) != CellDoor)
				continue;
			// Door correct if there is exacly 2 walls around and it is a left-right or up-down
			if(loc->around(v, CellWall, CellWall) == 2
				&& (isboth(v, Left, Right, CellWall, CellWall) || isboth(v, Up, Down, CellWall, CellWall)))
				continue;
			// Incorrect door must be eliminated
			loc->set(v, CellPassable);
			loc->removeov(v);
		}
	}
}

static int total_levels(slice<quest::leveli> source) {
	auto result = 0;
	for(auto& e : source)
		result += e.level;
	return result;
}

static bool is_valid(pointc v) {
	return pathmap[v.y][v.x] != 0 && pathmap[v.y][v.x] != 0xFFFF;
}

static bool is_valid_dungeon() {
	if(!loc->state.up)
		return true;
	loc->block(true);
	loc->makewave(loc->state.up);
	if(loc->state.down && !is_valid(loc->state.down))
		return false;
	for(auto v : loc->state.features) {
		if(!is_valid(v))
			return false;
	}
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
	if(!points)
		return pointc();
	auto n = points[0];
	points.remove(0);
	return n;
}

static bool test_shape(pointc v, directions d, const shapei* shape) {
	if(!shape)
		return true;
	pointc c;
	for(c.y = 0; c.y < shape->size.y; c.y++) {
		for(c.x = 0; c.x < shape->size.x; c.x++) {
			auto n = (*shape)[c];
			if(n == ' ')
				continue;
			auto v1 = shape->translate(v, c, d);
			if(!v1)
				return false;
			if(loc->get(v1) != CellUnknown)
				return false;
		}
	}
	return true;
}

static bool test_shape(pointc& v, directions d, const shapei* shape, int dx, int dy) {
	if(shape) {
		auto v1 = v.to(dx, dy);
		if(!test_shape(v1, d, shape))
			return false;
		v = v1;
	}
	return true;
}

static void apply_shape(pointc v, directions d, const shapei* shape, char sym, celln t) {
	pointc c;
	if(!shape)
		return;
	for(c.y = 0; c.y < shape->size.y; c.y++) {
		for(c.x = 0; c.x < shape->size.x; c.x++) {
			auto n = (*shape)[c];
			if(n == sym) {
				auto v1 = shape->translate(v, c, d);
				loc->set(v1, t);
			}
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

static void apply_shape(pointc v, directions d, const shapei* shape, char sym, corridori* pi, celln cell) {
	if(pi)
		apply_shape(v, d, shape, sym, pi->proc);
	else
		apply_shape(v, d, shape, sym, cell);
}

static void stairs_up(pointc v, directions d, const shapei* ps) {
	apply_shape(v, d, ps, '0', CellStairsUp);
	apply_shape(v, d, ps, '1', CellPassable);
	loc->state.up.d = d;
}

static void stairs_down(pointc v, directions d, const shapei* ps) {
	apply_shape(v, d, ps, '0', CellStairsDown);
	apply_shape(v, d, ps, '1', CellPassable);
	loc->state.down.d = d;
}

static void create_lair(pointc v, directions d, const shapei* ps) {
	apply_shape(v, d, ps, '0', monster_boss);
	apply_shape(v, d, ps, '1', lair_door);
	apply_shape(v, d, ps, '2', monster_minion);
	apply_shape(v, d, ps, '.', monster);
}

static void validate_position(pointc& v, directions d, const shapei* shape) {
	if(test_shape(v, d, shape))
		return;
	for(int r = 1; r < 5; r++) {
		if(rand() % 2) {
			if(test_shape(v, d, shape, r, 0))
				return;
			if(test_shape(v, d, shape, -r, 0))
				return;
			if(test_shape(v, d, shape, 0, r))
				return;
			if(test_shape(v, d, shape, 0, -r))
				return;
		} else {
			if(test_shape(v, d, shape, 0, r))
				return;
			if(test_shape(v, d, shape, 0, -r))
				return;
			if(test_shape(v, d, shape, r, 0))
				return;
			if(test_shape(v, d, shape, -r, 0))
				return;
		}
	}
}

static void create_room(pointc v, directions d, const char* id, fnroom proc) {
	if(!v)
		return;
	auto ps = bsdata<shapei>::find(id);
	if(!ps)
		return;
	validate_position(v, d, ps);
	apply_shape(v, d, ps, 'X', CellWall);
	apply_shape(v, d, ps, '.', CellPassable);
	proc(v, d, ps);
	put_corridor(ps->translate(v, ps->points[1], d), d, false);
#ifdef DEBUG_ROOM
	show_map_interactive();
#endif
}

static void create_room(pointc v, const char* id, fnroom proc) {
	create_room(v, optimal_direction(v), id, proc);
}

static void create_room_features(pointc v, directions d, roomi& ei) {
	for(auto i = 0; i < lenghtof(ei.features); i++)
		apply_shape(v, d, ei.shape, '0' + i, ei.features[i], CellPassable);
}

static void add_features(pointc v, directions d) {
	auto pv = loc->state.features.add();
	if(pv) {
		pv->x = v.x;
		pv->y = v.y;
		pv->d = d;
	}
}

static void create_room(pointc v, roomi& ei) {
	if(!v)
		return;
	directions d = optimal_direction(v);
	validate_position(v, d, ei.shape);
	apply_shape(v, d, ei.shape, 'X', CellWall);
	apply_shape(v, d, ei.shape, '.', ei.floor, CellPassable);
	create_room_features(v, d, ei);
	put_corridor(ei.shape->translate(v, ei.shape->points[1], d), d, false);
	add_features(ei.shape->translate(v, ei.shape->points[0], d), d);
#ifdef DEBUG_ROOM
	show_map_features();
#endif
}

static void create_rooms(pointca& points, const variants& features) {
	for(auto v : features) {
		if(v.counter > 0) {
			// Percent chance of room appear
			if(d100() >= v.counter)
				return;
		}
		v = single(v);
		if(v.iskind<roomi>())
			create_room(pop(points), bsdata<roomi>::elements[v.value]);
		else if(v.iskind<listi>())
			create_rooms(points, bsdata<listi>::elements[v.value].elements);
	}
}

static void create_rooms(pointc start, bool last_level, variants features) {
	pointca points;
	create_points(points, 3, 3, 2);
	if(start)
		create_room(start, "ShapeExit", stairs_up);
	else
		create_room(pop(points), "ShapeExit", stairs_up);
	if(!last_level)
		create_room(pop(points), "ShapeExit", stairs_down);
	create_rooms(points, features);
}

static void drop_special_item() {
	if(!loc->special)
		return;
	if(loc->state.special)
		return;
	item it; it.create(loc->special);
	it.set(QuestItem);
	if(d100() < 60)
		it.createpower(xrand(2, 5), 100, 5);
	pointc v;
	if(it) {
		pointca points;
		for(auto& e : loc->items) {
			if(!e || e.is(Quiver) || e.is(Edible))
				continue;
			points.addu(e);
		}
		if(points)
			v = points.random();
	}
	if(!v && loc->state.portal)
		v = loc->state.portal;
	if(!v && loc->state.down)
		v = loc->state.down;
	if(!v && loc->state.up)
		v = loc->state.up;
	if(v) {
		loc->drop(v, it, xrand(0, 3));
		loc->state.special = v;
		loc->state.wallmessages[MessageSpecialItem]++;
	}
}

static void link_dungeon(dungeoni& upper, dungeoni& current) {
	pointc v;
	unsigned short pm1[mpy][mpx];
	unsigned short pm2[mpy][mpx];
	// 1) Get idicies of two linked dungeons
	current.block(true);
	current.makewave(to(current.state.up, current.state.up.d));
	memcpy(pm1, pathmap, sizeof(pathmap));
	upper.block(true);
	upper.makewave(to(upper.state.down, upper.state.down.d));
	memcpy(pm2, pathmap, sizeof(pathmap));
	// 2) Get valid indicies
	for(v.y = 0; v.y < mpy; v.y++) {
		for(v.x = 0; v.x < mpx; v.x++) {
			// Dungeon must do not have monster in cell
			if(current.ismonster(v) || upper.ismonster(v))
				pm2[v.y][v.x] = 0xFFFF;
			// Dungeon must not have door in this cell (door cell is passable)
			if(current.get(v) == CellDoor || upper.get(v) == CellDoor)
				pm2[v.y][v.x] = 0xFFFF;
			if(current.get(v) == CellPit || upper.get(v) == CellPit)
				pm2[v.y][v.x] = 0xFFFF;
			// There is no location right before stairs
			if(v == to(upper.state.down, upper.state.down.d)
				|| v == to(upper.state.up, upper.state.up.d)
				|| v == to(current.state.up, current.state.up.d)
				|| v == to(current.state.down, current.state.down.d))
				pm2[v.y][v.x] = 0xFFFF;
			// Dungeon must be passable
			if(!pm1[v.y][v.x] || !pm2[v.y][v.x] || pm2[v.y][v.x] >= 0xFF00)
				pm1[v.y][v.x] = 0xFFFF;
		}
	}
	// 3) Get possible pits indicies
	pointca points;
	for(v.y = 0; v.y < mpy; v.y++) {
		for(v.x = 0; v.x < mpx; v.x++) {
			if(pm1[v.y][v.x] && pm1[v.y][v.x] < 0xFF00)
				points.add(v);
		}
	}
	// 4) Place random count of pits (usually for 1 to 4)
	zshuffle(points.data, points.count);
	points.top(xrand(1, 4));
	for(auto v : points)
		upper.set(v, CellPit);
}

static void select_points(pointca& result, bool(*fnfilter)(pointc)) {
	pointc m;
	for(m.x = 0; m.x < mpx; m.x++) {
		for(m.y = 0; m.y < mpy; m.y++) {
			if(fnfilter(m))
				result.add(m);
		}
	}
}

static void add_special(pointca& points, celln t, int minimum, int maximum) {
	if(!maximum)
		maximum = minimum;
	if(!minimum)
		return;
	for(auto count = xrand(minimum, maximum); count > 0; count--) {
		auto v = pop(points);
		if(!v)
			break;
		loc->set(v, t);
	}
}

static bool is_corridor(pointc v) {
	if(!v)
		return false;
	return (isboth(v, Up, Down, CellPassable, CellPassable) && isboth(v, Left, Right, CellWall, CellWall))
		|| (isboth(v, Left, Right, CellPassable, CellPassable) && isboth(v, Up, Down, CellWall, CellWall));
}

static bool is_empthy_corridor(pointc v) {
	if(loc->get(v) != CellPassable)
		return false;
	if(!is_corridor(v))
		return false;
	if(loc->ismonster(v))
		return false;
	if(loc->isitem(v))
		return false;
	if(loc->isoverlay(v))
		return false;
	return true;
}

static bool is_empthy_corner(pointc v) {
	if(loc->get(v) != CellPassable)
		return false;
	if(loc->around(v, CellWall, CellWall) != 3)
		return false;
	if(loc->around(v, CellPassable, CellPassable) != 1)
		return false;
	if(loc->ismonster(v))
		return false;
	if(loc->isitem(v))
		return false;
	if(loc->isoverlay(v))
		return false;
	return true;
}

static void create_dungeon_objects() {
	pointca points;
	select_points(points, is_empthy_corridor);
	points.shuffle();
	if(loc->webs)
		add_special(points, CellWeb, loc->webs, loc->webs * 2);
	if(loc->barrels)
		add_special(points, CellBarel, loc->barrels / 2, loc->barrels);
	points.clear();
	select_points(points, is_empthy_corner);
	if(loc->graves)
		add_special(points, CellGrave, loc->graves, loc->graves);
	if(loc->eggs)
		add_special(points, CellCocon, loc->eggs, loc->eggs);
}

static void dungeon_create(unsigned short quest_id, slice<quest::leveli> source) {
	auto base = 1;
	auto total_level_count = total_levels(source);
	dungeoni* previous = 0;
	dungeoni* start = 0;
	for(auto& ei : source) {
		if(!ei || !ei.level)
			continue;
		auto special_item_level = -1;
		if(ei.special)
			special_item_level = rand() % ei.level;
		for(auto j = 0; j < ei.level; j++) {
			loc = bsdata<dungeoni>::add();
			if(!start)
				start = loc;
			auto level = base + j;
			posable start;
			if(previous)
				start = previous->state.down;
			auto last_level = (level == total_level_count);
			while(true) {
				loc->clear();
				assign<dungeon_site>(*loc, ei);
				loc->quest_id = quest_id;
				loc->level = level;
				loc->cursed = 5;
				create_rooms(start, last_level, ei.features);
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
			resolve_traps();
			if(special_item_level == j)
				drop_special_item();
			else
				loc->special = 0;
			create_dungeon_objects();
#ifdef DEBUG_DUNGEON
			show_map_pathfind();
#endif
			loc->state.total_passable = loc->getpassables(false);
			previous = loc;
		}
		base += ei.level;
	}
	auto finish = loc;
	// Add dungeon pits and other stuff
	if(start) {
		for(auto p = start; p < finish; p++)
			link_dungeon(p[0], p[1]);
	}
}

void dungeon_create() {
	auto push_loc = loc;
	dungeon_create(getbsi(last_quest), last_quest->sites);
	loc = push_loc;
}

BSDATA(corridori) = {
	{"Empthy", empthy},
	{"Boss", monster_boss},
	{"Cellar", cellar},
	{"Decoration", decoration},
	{"Door", lair_door},
	{"FloorRation", rations},
	{"FloorStones", stones},
	{"FloorTrap", trap},
	{"FloorTreasure", floor_treasure},
	{"Message", message},
	{"Minions", monster_minion},
	{"Passable", corridor_passable},
	{"Portal", portal},
	{"Prison", prison},
	{"Secret", secret},
	{"StairsDown", corridor_stairs_down},
	{"StairsUp", corridor_stairs_up},
	{"Treasure", treasure},
	{"WanderingMonster", monster},
};
BSDATAF(corridori)