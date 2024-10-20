#include "assign.h"
#include "cell.h"
#include "direction.h"
#include "dungeon.h"
#include "math.h"
#include "reference.h"

const unsigned char	CellMask = 0x1F;
const unsigned short Blocked = 0xFFFF;
static pointc path_stack[256];
static unsigned char path_push;
static unsigned char path_pop;
static directions all_directions[] = {Up, Right, Down, Left};
unsigned short pathmap[mpy][mpx];

dungeoni *loc, *locup, *locdw;

static void snode(pointc v, short unsigned cost) {
	if(!v)
		return;
	auto a = pathmap[v.y][v.x];
	if(a != Blocked && (!a || cost < a)) {
		path_stack[path_push++] = v;
		pathmap[v.y][v.x] = cost;
	}
}

static celln get_wall(celln v) {
	switch(v) {
	case CellSecrectButton:
	case CellPortal:
	case CellStairsUp:
	case CellStairsDown:
		return CellWall;
	default:
		return v;
	}
}

template<> referencei::referencei(creaturei* p) {
	if(bsdata<creaturei>::have(p)) {
		parent = 0xFFFF;
		index = getbsi(p);
	} else if(loc && loc->have(p)) {
		parent = getbsi(loc);
		index = p - loc->monsters;
	} else {
		parent = 0xFFFF;
		index = 0xFFFF;
	}
}

void dungeoni::overlayi::clear() {
	posable::clear();
	link.clear();
	type = CellUnknown;
	subtype = 0;
}

void dungeoni::ground::clear() {
	posable::clear();
	item::clear();
}

void dungeoni::clear() {
	memset(this, 0, sizeof(*this));
	state.clear();
	for(auto& e : overlays)
		e.clear();
	for(auto& e : items)
		e.clear();
	for(auto& e : monsters)
		e.clear();
}

void dungeoni::change(celln s, celln d) {
	pointc v;
	for(v.y = 0; v.y < mpy; v.y++) {
		for(v.x = 0; v.x < mpx; v.x++) {
			if(get(v) == s)
				set(v, d);
		}
	}
}

void dungeoni::block(bool treat_door_as_passable) const {
	pointc v;
	for(v.y = 0; v.y < mpy; v.y++) {
		for(v.x = 0; v.x < mpx; v.x++) {
			switch(get(v)) {
			case CellWall:
			case CellPortal:
			case CellUnknown:
				pathmap[v.y][v.x] = Blocked;
				break;
			case CellDoor:
				if(!treat_door_as_passable && !is(v, CellActive))
					pathmap[v.y][v.x] = Blocked;
				else
					pathmap[v.y][v.x] = 0;
				break;
			default:
				pathmap[v.y][v.x] = 0;
				break;
			}
		}
	}
}

void dungeoni::makewave(pointc start) const {
	if(!start)
		return;
	path_push = path_pop = 0;
	path_stack[path_push++] = start;
	pathmap[start.y][start.x] = 1;
	while(path_push != path_pop) {
		auto v = path_stack[path_pop++];
		auto cost = pathmap[v.y][v.x] + 1;
		if(cost >= 0xFF00)
			break;
		snode(to(v, Left), cost);
		snode(to(v, Right), cost);
		snode(to(v, Up), cost);
		snode(to(v, Down), cost);
	}
}

celln dungeoni::get(pointc v) const {
	if(!v)
		return CellWall;
	return (celln)((data[v.y][v.x] & CellMask));
}

dungeoni::overlayi* dungeoni::add(pointc v, directions d, celln i) {
	if(!v)
		return 0;
	for(auto& e : overlays) {
		if(!e) {
			e.clear();
			assign<pointc>(e, v);
			e.d = d;
			e.type = i;
			return &e;
		}
	}
	return 0;
}

void dungeoni::add(overlayi* po, item& it) {
	if(!it || !po)
		return;
	if(!have(po))
		return;
	for(auto& e : overlayitems) {
		if(!e) {
			e.clear();
			e.storage_index = po - overlays;
			assign<item>(e, it);
			it.clear();
			last_item = &e;
			break;
		}
	}
}

void dungeoni::addmonster(pointc v, directions d, int side, const monsteri* pi) {
	player = 0;
	for(auto& e : monsters) {
		if(e)
			continue;
		player = &e;
		create_monster(pi);
		e.x = v.x;
		e.y = v.y;
		e.d = d;
		e.side = side;
		state.monsters++;
		break;
	}
}

dungeoni::overlayi* dungeoni::get(pointc v, directions d) {
	if(!v)
		return 0;
	for(auto& e : overlays) {
		if(e == v && e.d == d)
			return &e;
	}
	return 0;
}

dungeoni::overlayi* dungeoni::getoverlay(pointc v, celln type) {
	for(auto& e : overlays) {
		if(e == v && e.type == type)
			return &e;
	}
	return 0;
}

void dungeoni::set(pointc v, celln i) {
	if(!v)
		return;
	switch(i) {
	case CellPortal: state.portal = v; break;
	case CellStairsUp: state.up = v; break;
	case CellStairsDown: state.down = v; break;
	default: break;
	}
	data[v.y][v.x] = (data[v.y][v.x] & (~CellMask)) | i;
}

void dungeoni::set(pointc v, celln i, pointc size) {
	auto ve = v + size; pointc pt;
	for(pt.y = v.y; pt.y < ve.y; pt.y++)
		for(pt.x = v.x; pt.x < ve.x; pt.x++)
			set(pt, i);
}

bool dungeoni::is(pointc v, cellfn i) const {
	if(!v)
		return false;
	return (data[v.y][v.x] & (0x80 >> i)) != 0;
}

bool dungeoni::is(pointc v, celln t1, celln t2) const {
	if(!v)
		return true;
	auto t = get(v);
	return t == t1 || t == t2;
}

void dungeoni::remove(pointc v, cellfn i) {
	if(!v)
		return;
	data[v.y][v.x] &= ~(0x80 >> i);
}

void dungeoni::removeov(pointc v) {
	if(!v)
		return;
	for(auto& e : overlays) {
		if(e.d == Center)
			continue;
		if(to(e, e.d) == v)
			e.clear();
	}
}

void dungeoni::set(pointc v, cellfn i) {
	if(!v)
		return;
	data[v.y][v.x] |= 0x80 >> i;
}

void dungeoni::set(pointc v, celln type, directions d) {
	set(to(v, to(d, Left)), CellWall);
	set(to(v, to(d, Right)), CellWall);
	set(to(v, to(d, Down)), CellWall);
	set(v, type);
	switch(type) {
	case CellStairsUp: state.up.set(v, d); break;
	case CellStairsDown: state.down.set(v, d); break;
	case CellPortal: state.portal.set(v, d); break;
	default: break;
	}
}

bool dungeoni::ismonster(pointc v) const {
	for(auto& e : monsters) {
		if(e && e == v)
			return true;
	}
	return false;
}

bool dungeoni::ispassable(pointc v) const {
	if(!v)
		return false;
	switch(get(v)) {
	case CellPassable:
	case CellButton:
	case CellWebTorned:
		return true;
	case CellDoor:
		return is(v, CellActive);
	default:
		return false;
	}
}

int dungeoni::around(pointc v, celln t1, celln t2) const {
	auto result = 0;
	for(auto d : all_directions) {
		auto t = loc->get(to(v, d));
		if(t == t1 || t == t2)
			result++;
	}
	return result;
}

void dungeoni::drop(pointc v, item& it, int side) {
	if(!it)
		return;
	for(auto& e : items) {
		if(e)
			continue;
		assign<item>(e, it);
		e.x = v.x;
		e.y = v.y;
		e.side = side;
		e.d = Center;
		it.clear();
		last_item = &e;
		auto index = &e - items + 1;
		if(state.items < index)
			state.items = index;
		break;
	}
}

size_t dungeoni::getitems(ground** result, size_t result_maximum, pointc v) {
	auto ps = result;
	auto pe = ps + result_maximum;
	for(auto& e : items) {
		if(!e || e != v)
			continue;
		if(ps < pe)
			*ps++ = &e;
		else
			break;
	}
	return ps - result;
}

size_t dungeoni::getitems(item** result, size_t result_maximum, const overlayi* po) {
	if(!have(po))
		return 0;
	auto index = po - overlays;
	auto ps = result;
	auto pe = ps + result_maximum;
	for(auto& e : overlayitems) {
		if(!e || e.storage_index != index)
			continue;
		if(ps < pe)
			*ps++ = &e;
		else
			break;
	}
	return ps - result;
}

void dungeoni::getmonsters(creaturei** result, pointc index, directions dr) {
	result[0] = result[1] = result[2] = result[3] = 0;
	if(!index)
		return;
	for(auto& e : monsters) {
		if(!e)
			continue;
		if(e != index)
			continue;
		if(e.is(Large))
			result[2] = &e;
		else
			result[get_side(e.side, dr)] = &e;
	}
}

void dungeoni::getmonsters(creaturei** result, pointc index) {
	getmonsters(result, index, Center);
}

bool filter_corridor(pointc v) {
	if(get_wall(loc->get(v)) == CellWall)
		return false;
	return (get_wall(loc->get(to(v, Up))) == CellWall && get_wall(loc->get(to(v, Down))) == CellWall)
		|| (get_wall(loc->get(to(v, Left))) == CellWall && get_wall(loc->get(to(v, Right))) == CellWall);
}

const char* get_part_placement(pointc v) {
	static const char* names[] = {
		"NorthWest", "North", "NorthEast",
		"West", "Central", "East",
		"SouthWest", "South", "SouthEast"
	};
	auto dx = imax(0, imin(2, v.x / (mpx / 3)));
	auto dy = imax(0, imin(2, (v.y / (mpy / 3)) * 3));
	return names[dy * 3 + dx];
}

int get_side(int side, directions d) {
	static const char place_sides[4][4] = {
		{1, 3, 0, 2},
		{0, 1, 2, 3},
		{2, 0, 3, 1},
		{3, 2, 1, 0},
	};
	if(d == Center)
		return side;
	return place_sides[d - Left][side];
}