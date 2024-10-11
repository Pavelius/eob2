#include "cell.h"
#include "direction.h"
#include "dungeon.h"

const unsigned char	CellMask = 0x1F;
const unsigned short Blocked = 0xFFFF;
static unsigned short pathmap[mpy][mpx];

dungeoni *loc, *locup, *locdw;

static celln get_wall(celln v) {
	switch(v) {
	case CellSecrectButton:
	case CellPortal:
		return CellWall;
	default:
		return v;
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

celln dungeoni::get(pointc v) const {
	if(!v)
		return CellWall;
	return (celln)((data[v.y][v.x] & CellMask));
}

dungeoni::overlayi* dungeoni::add(pointc v, celln i, directions d) {
	if(!v)
		return 0;
	return 0;
}

dungeoni::overlayi* dungeoni::getoverlay(pointc v, directions d) {
	return 0;
}

void dungeoni::set(pointc v, celln i) {
	if(!v)
		return;
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