#include "cell.h"
#include "dungeon.h"
#include "direction.h"
#include "math.h"
#include "rand.h"
#include "resid.h"
#include "wallmessage.h"

const int EmpthyStartIndex = 1;

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

static void items(pointc v, int bonus_level) {
	//items(pd, index, random_type(), bonus_level);
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
	//for(int i = random_prison_count(); i > 0; i--)
	//	items(pd, i2, 0);
	monster(v2, Down, 0);
	loc->set(to(v2, to(d, Left)), CellWall);
	loc->set(to(v2, to(d, Right)), CellWall);
	loc->set(to(v2, to(d, Up)), CellWall);
}

static void treasure(pointc v, directions d, unsigned flags) {
	if(loc->type == FOREST)
		return;
	//if(need_crypt_button(pd, index, dir, flags))
	//	return;
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
	loc->add(to(v, to(d, Right)), CellKeyHole, d);
	loc->set(to(v1, to(d, Left)), CellWall);
	loc->set(to(v1, to(d, Right)), CellWall);
	loc->set(v2, CellPassable);
	//for(auto i = 1 + random_prison_count(); i > 0; i--)
	//	items(pd, i2, magic_bonus);
	loc->set(to(v2, to(d, Left)), CellWall);
	loc->set(to(v2, to(d, Right)), CellWall);
	loc->set(to(v2, to(d, Up)), CellWall);
}

static void decoration(pointc v, directions d, unsigned flags) {
	if(loc->type == FOREST)
		return;
	//if(need_crypt_button(pd, index, dir, flags))
	//	return;
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
	po->subtype = loc->state.messages;
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
	//items(pd, index, Ration, 0);
}

static void stones(pointc v, directions d, unsigned flags) {
	//items(pd, index, Stone, 0);
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
		if(!new_index|| loc->get(new_index) != CellUnknown)
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