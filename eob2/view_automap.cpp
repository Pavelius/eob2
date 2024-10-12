#include "cell.h"
#include "direction.h"
#include "draw.h"
#include "dungeon.h"
#include "party.h"
#include "view.h"

using namespace draw;

static color cpass(196, 132, 72);
static color cwall(156, 104, 54);
static color bwall(100, 64, 24);
static color bpass(176, 120, 64);
static color bpits = bpass.darken();
static color cdoor(140, 88, 48);
static bool show_fog_of_war = false;

const int mpg = 8;

static point gs(int x, int y) {
	return {(short)(x * mpg + mpg), (short)(y * mpg + mpg)};
}

static void red_marker() {
	auto push_fore = fore; fore = colors::red;
	auto push_caret = caret;
	line(caret.x + mpg, caret.y + mpg);
	caret.x = push_caret.x + mpg;
	caret.y = push_caret.y;
	line(caret.x, caret.y + mpg);
	caret = push_caret;
	fore = push_fore;
}

static void show_camera_pos() {
	auto push_caret = caret;
	auto push_fore = fore;
	fore = colors::red;
	auto direct = party.d;
	auto camera = gs(party.x, party.y);
	auto x1 = camera.x;
	auto y1 = camera.y;
	auto x2 = x1 + mpg;
	auto y2 = y1 + mpg;
	auto cx = x1 + mpg / 2;
	auto cy = y1 + mpg / 2;
	switch(direct) {
	case Left:
		caret.x = x2; caret.y = cy;
		line(x1, cy);
		pixel(caret.x + 1, caret.y - 1);
		pixel(caret.x + 1, caret.y + 1);
		break;
	case Right:
		caret.x = x1; caret.y = cy;
		line(x2, cy);
		pixel(caret.x - 1, caret.y - 1);
		pixel(caret.x - 1, caret.y + 1);
		break;
	case Up:
		caret.x = cx; caret.y = y2;
		line(caret.x, y1);
		pixel(caret.x - 1, caret.y + 1);
		pixel(caret.x + 1, caret.y + 1);
		break;
	case Down:
		caret.x = cx; caret.y = 12;
		line(caret.x, y2);
		pixel(caret.x - 1, caret.y - 1);
		pixel(caret.x + 1, caret.y - 1);
		break;
	}
	fore = push_fore;
	caret = push_caret;
}

static celln tget(int x, int y) {
	if(show_fog_of_war && !loc->is({(char)x, (char)y}, CellExplored))
		return CellWall;
	auto t = loc->get({(char)x, (char)y});
	switch(t) {
	case CellUnknown:
	case CellPortal:
		return CellWall;
	default:
		return t;
	}
}

static void fill_neighboard(pointc v, celln* nb) {
	nb[0] = tget(v.x - 1, v.y);
	nb[1] = tget(v.x, v.y - 1);
	nb[2] = tget(v.x + 1, v.y);
	nb[3] = tget(v.x, v.y + 1);
	//
	nb[4] = tget(v.x - 1, v.y + 1);
	nb[5] = tget(v.x - 1, v.y - 1);
	nb[6] = tget(v.x + 1, v.y - 1);
	nb[7] = tget(v.x + 1, v.y + 1);
}

static void fill_side(int dx, color border, celln* nb, celln t1) {
	auto pos = caret;
	auto push_fore = fore; fore = border;
	int x2 = caret.x + mpg - 1;
	int y2 = caret.y + mpg - 1;
	if(nb[0] == t1) {
		caret.x = pos.x + dx;
		caret.y = pos.y;
		line(pos.x + dx, y2);
	} else if(nb[1] == t1) {
		caret.x = pos.x;
		caret.y = pos.y + dx;
		line(x2, pos.y + dx);
	} else if(nb[2] == t1) {
		caret.x = x2 - dx;
		caret.y = pos.y;
		line(x2 - dx, y2);
	} else if(nb[3] == t1) {
		caret.x = pos.x;
		caret.y = y2 - dx;
		line(x2, y2 - dx);
	}
	caret = pos;
	fore = push_fore;
}

static void fill_border(int dx, color border, celln* nb, celln t1) {
	int x2 = caret.x + mpg - 1;
	int y2 = caret.y + mpg - 1;
	auto push_fore = fore; fore = border;
	auto push_caret = caret;
	if(nb[0] != t1) {
		caret.x = push_caret.x + dx;
		caret.y = push_caret.y;
		line(push_caret.x + dx, y2);
	}
	if(nb[1] != t1) {
		caret.x = push_caret.x;
		caret.y = push_caret.y + dx;
		line(x2, push_caret.y + dx);
	}
	if(nb[2] != t1) {
		caret.x = x2 - dx;
		caret.y = push_caret.y;
		line(x2 - dx, y2);
	}
	if(nb[3] != t1) {
		caret.x = push_caret.x;
		caret.y = y2 - dx;
		line(x2, y2 - dx);
	}
	if(dx) {
		if(nb[0] == t1 && nb[1] == t1 && nb[5] != t1)
			pixel(push_caret.x, push_caret.y);
		if(nb[1] == t1 && nb[2] == t1 && nb[6] != t1)
			pixel(x2, push_caret.y);
		if(nb[2] == t1 && nb[3] == t1 && nb[7] != t1)
			pixel(x2, y2);
		if(nb[0] == t1 && nb[3] == t1 && nb[4] != t1)
			pixel(push_caret.x, y2);
	}
	fore = push_fore;
	caret = push_caret;
}

static void rectf(int sx, int sy) {
	auto push_width = width;
	auto push_height = height;
	width = sx; height = sy;
	rectf();
	width = push_width;
	height = push_height;
}

static void paint_background() {
	rectpush push;
	fore = cpass;
	caret.x = 0;
	caret.y = 0;
	width = 320;
	height = 200;
	rectf();
}

static void paint_automap() {
	rectpush push;
	auto push_fore = fore;
	celln nb[8];
	paint_background();
	// render_overlays(location, fog_of_war);
	pointc v;
	for(v.y = -1; v.y < mpy + 1; v.y++) {
		for(v.x = -1; v.x < mpx + 1; v.x++) {
			if(!v)
				continue;
			//if(fog_of_war) {
			//	if(index == Blocked) {
			//		auto x1 = imax(0, imin(x, mpx - 1));
			//		auto y1 = imax(0, imin(y, mpy - 1));
			//		if(!location.is(location.getindex(x1, y1), CellExplored))
			//			continue;
			//	} else {
			//		if(!location.is(index, CellExplored))
			//			continue;
			//	}
			//}
			fill_neighboard(v, nb);
			auto pos = gs(v.x, v.y);
			caret = pos;
			width = mpg; height = mpg;
			switch(loc->get(v)) {
			case CellUnknown:
				fore = cwall; rectf();
				break;
			case CellWall:
				fore = cwall; rectf();
				fill_border(1, cdoor, nb, CellWall);
				fill_border(0, bwall, nb, CellWall);
				break;
			case CellPit:
				setoffset(1, 1);
				fore = cwall; rectf();
				fore = bpits; rectb();
				break;
			case CellButton:
				setoffset(1, 1);
				fore = bpass; rectf();
				fore = bpits; rectb();
				break;
			case CellDoor:
				fore = cdoor;
				if(nb[0] == CellWall && nb[2] == CellWall) {
					auto yc = pos.y + mpg / 2 - 1;
					caret.x = pos.x; caret.y = yc;
					line(caret.x + mpg, yc); yc++;
					caret.x = pos.x; caret.y = yc;
					line(caret.x + mpg, yc); yc++;
				} else {
					auto xc = pos.x + mpg / 2 - 1;
					caret.x = xc; caret.y = pos.y;
					line(xc, pos.y + mpg); xc++;
					caret.x = xc; caret.y = pos.y;
					line(xc, pos.y + mpg); xc++;
				}
				break;
			case CellPortal:
				fore = cwall; rectf();
				fill_border(1, cdoor, nb, CellWall);
				fill_border(0, bwall, nb, CellWall);
				fill_side(2, bwall, nb, CellPassable);
				break;
			case CellStairsUp:
			case CellStairsDown:
				fore = cdoor;
				if(nb[0] == CellWall && nb[2] == CellWall) {
					auto yc = pos.y + 1;
					caret.x = pos.x; caret.y = yc;
					line(pos.x + mpg, yc); yc += 2;
					caret.x = pos.x; caret.y = yc;
					line(pos.x + mpg, yc); yc += 2;
					caret.x = pos.x; caret.y = yc;
					line(pos.x + mpg, yc); yc += 2;
				} else {
					auto xc = pos.x + 1;
					caret.x = xc; caret.y = pos.y;
					line(xc, pos.y + mpg); xc += 2;
					caret.x = xc; caret.y = pos.y;
					line(xc, pos.y + mpg); xc += 2;
					caret.x = xc; caret.y = pos.y;
					line(xc, pos.y + mpg); xc += 2;
				}
				break;
			}
		}
	}
	fore = push_fore;
}

static void input_automap() {
	switch(hot.key) {
	case KeyEscape:
	case KeySpace:
		breakmodal(0);
		break;
	}
}

void show_automap(bool fog_of_war) {
	auto push_visible = show_fog_of_war;
	show_fog_of_war = fog_of_war;
	show_scene(paint_automap, input_automap, 0);
	show_fog_of_war = push_visible;
}