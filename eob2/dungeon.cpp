#include "cell.h"
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
	if(v.x < 0)
		return CellWall;
	return (celln)((data[v.y][v.x] & CellMask));
}

void dungeoni::set(pointc v, celln i) {
	if(v.x < 0)
		return;
	data[v.y][v.x] = (data[v.y][v.x] & (~CellMask)) | i;
}

bool dungeoni::is(pointc v, cellfn i) const {
	if(v.x < 0)
		return false;
	return (data[v.y][v.x] & (0x80 >> i)) != 0;
}

void dungeoni::remove(pointc v, cellfn i) {
	if(v.x < 0)
		return;
	data[v.y][v.x] &= ~(0x80 >> i);
}

void dungeoni::set(pointc v, cellfn i) {
	if(v.x < 0)
		return;
	data[v.y][v.x] |= 0x80 >> i;
}