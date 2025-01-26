#include "cell.h"
#include "direction.h"
#include "draw.h"
#include "dungeon.h"
#include "math.h"
#include "party.h"
#include "pointca.h"
#include "screenshoot.h"
#include "view.h"

using namespace draw;

static color cpass(196, 132, 72);
static color cegg(119, 141, 97);
static color cwall(156, 104, 54);
static color cbarrel(147, 73, 0);
static color cdoor(140, 88, 48);
static color cweb(162, 171, 183);
static color bwall(100, 64, 24);
static color bpass(176, 120, 64);
static color bpits = bpass.darken();
static const pointca* red_markers;
static bool show_fog_of_war;
static bool show_secrets;
static bool show_party;

const int mpg = 8;

static point gs(int x, int y) {
	return {(short)(x * mpg + mpg), (short)(y * mpg + mpg)};
}

static void paint_marker() {
	auto push_caret = caret;
	line(caret.x + mpg, caret.y + mpg);
	caret.x = push_caret.x + mpg;
	caret.y = push_caret.y;
	line(push_caret.x, push_caret.y + mpg);
	caret = push_caret;
}

static void red_marker() {
	auto push_fore = fore;
	fore = colors::red;
	paint_marker();
	fore = push_fore;
}

void paint_cross(int offset) {
	rectpush push;
	caret.x = push.caret.x + width / 2;
	caret.y = push.caret.y + offset;
	line(caret.x, push.caret.y + height - offset);
	caret.x = push.caret.x + offset;
	caret.y = push.caret.y + height / 2;
	line(caret.x + width - offset - 2, caret.y);
}

static void paint_bold(int x, int y) {
	pixel(x, y);
	pixel(x + 1, y);
	pixel(x, y + 1);
	pixel(x + 1, y + 1);
}

void paint_arrow(point camera, directions direct, int mpg) {
	auto x1 = camera.x;
	auto y1 = camera.y;
	auto x2 = x1 + mpg - 1;
	auto y2 = y1 + mpg - 1;
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
		caret.x = cx; caret.y = y1;
		line(caret.x, y2);
		pixel(caret.x - 1, caret.y - 1);
		pixel(caret.x + 1, caret.y - 1);
		break;
	}
}

static void paint_party_position() {
	auto push_caret = caret;
	auto push_fore = fore;
	fore = colors::red;
	paint_arrow(gs(party.x, party.y), party.d, mpg);
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

static void fill_line(directions d, int dx, int size, color border) {
	auto push_fore = fore; fore = border;
	auto pos = caret;
	switch(d) {
	case Left:
		caret.x = pos.x - dx;
		caret.y = pos.y + (mpg - size) / 2;
		line(caret.x, caret.y + size);
		break;
	case Right:
		caret.x = pos.x + mpg - 1 + dx;
		caret.y = pos.y + (mpg - size) / 2;
		line(caret.x, caret.y + size);
		break;
	case Up:
		caret.x = pos.x + (mpg - size) / 2;
		caret.y = pos.y - dx;
		line(caret.x + size, caret.y);
		break;
	case Down:
		caret.x = pos.x + (mpg - size) / 2;
		caret.y = pos.y + mpg - 1 + dx;
		line(caret.x + size, caret.y);
		break;
	}
	caret = pos;
	fore = push_fore;
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
	auto push_fore = fore;
	fore = cpass;
	caret.x = 0;
	caret.y = 0;
	width = 320;
	height = 200;
	rectf();
	fore = push_fore;
}

static void paint_points(const pointca* source, fnevent proc) {
	if(!source)
		return;
	auto push_caret = caret;
	for(auto v : *source) {
		if(!v)
			continue;
		//if(show_fog_of_war && !loc->is(v, CellExplored))
		//	continue;
		caret = gs(v.x, v.y);
		proc();
	}
	caret = push_caret;
}

static void paint_automap() {
	rectpush push;
	auto push_fore = fore;
	celln nb[8]; pointc v;
	for(v.y = -1; v.y < mpy + 1; v.y++) {
		for(v.x = -1; v.x < mpx + 1; v.x++) {
			if(show_fog_of_war) {
				if(!v) {
					auto x1 = (char)imax(0, imin((int)v.x, mpx - 1));
					auto y1 = (char)imax(0, imin((int)v.y, mpy - 1));
					if(!loc->is({x1, y1}, CellExplored))
						continue;
				} else {
					if(!loc->is(v, CellExplored))
						continue;
				}
			}
			fill_neighboard(v, nb);
			auto pos = gs(v.x, v.y);
			caret = pos;
			width = mpg; height = mpg;
			auto t = loc->get(v);
			switch(t) {
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
			case CellBarel:
			case CellBarelDestroyed:
				setoffset(2, 2);
				fore = cbarrel;
				if(t == CellBarelDestroyed)
					fore = fore.mix(cpass);
				rectf();
				break;
			case CellCocon:
			case CellCoconOpened:
				setoffset(2, 2);
				fore = cegg;
				if(t == CellBarelDestroyed)
					fore = fore.mix(cpass);
				rectf();
				break;
			case CellGrave:
			case CellGraveDesecrated:
				fore = cbarrel;
				if(t == CellGraveDesecrated)
					fore = fore.mix(cpass);
				paint_cross(2);
				break;
			case CellOverlay1:
			case CellOverlay2:
			case CellOverlay3:
				fore = cbarrel;
				paint_marker();
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
			case CellWeb:
				fore = cweb;
				if(nb[0] == CellWall && nb[2] == CellWall) {
					auto yc = pos.y + mpg / 2 - 1;
					caret.x = pos.x; caret.y = yc;
					line(caret.x + mpg, yc); yc++;
				} else {
					auto xc = pos.x + mpg / 2 - 1;
					caret.x = xc; caret.y = pos.y;
					line(xc, pos.y + mpg); xc++;
				}
				break;
			case CellWebTorned:
				fore = cweb;
				if(nb[0] == CellWall && nb[2] == CellWall) {
					auto yc = pos.y + mpg / 2 - 1;
					pixel(pos.x, yc);
					pixel(pos.x + mpg - 1, yc);
				} else {
					auto xc = pos.x + mpg / 2 - 1;
					pixel(xc, pos.y);
					pixel(xc, pos.y + mpg - 1);
				}
				break;
			case CellPortal:
				fore = cwall; rectf();
				fill_border(1, cdoor, nb, CellWall);
				fill_border(0, bwall, nb, CellWall);
				fill_side(2, bwall, nb, CellPassable);
				break;
			case CellBloodBlades:
				fore = cwall;
				pixel(caret.x + 2, caret.y + 2);
				pixel(caret.x + mpg - 3, caret.y + 2);
				pixel(caret.x + 2, caret.y + mpg - 3);
				pixel(caret.x + mpg - 3, caret.y + mpg - 3);
				break;
			case CellJugDestroyed:
				fore = bpass;
				setoffset(2, 2);
				rectf();
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
			default:
				break;
			}
		}
	}
	fore = push_fore;
}

static void paint_overlays() {
	rectpush push;
	auto push_fore = fore;
	auto push_font = font;
	set_small_font();
	pointc v;
	for(auto& e : loc->overlays) {
		if(!e)
			continue;
		if(show_fog_of_war && !loc->is(e, CellExplored))
			continue;
		width = height = mpg;
		auto v = to(e, e.d);
		auto p1 = gs(v.x, v.y);
		auto p2 = gs(e.x, e.y);
		caret = p1;
		switch(e.type) {
		case CellSecretButton:
			if(show_fog_of_war && !loc->is(v, CellExplored))
				continue;
			if(show_secrets) {
				caret.x += 1; caret.y += 1;
				fore = bwall;
				text("X");
			}
			break;
		case CellTrapLauncher:
			if(show_fog_of_war && !loc->is(v, CellExplored))
				continue;
			caret = p2;
			fill_line(e.d, 2, 1, bwall);
			fill_line(e.d, 3, 1, cdoor);
			break;
		case CellCellar:
			if(show_fog_of_war && !loc->is(v, CellExplored))
				continue;
			caret = p2;
			fill_line(e.d, 2, 4, bwall);
			fill_line(e.d, 3, 4, cdoor);
			break;
		default:
			break;
		}
	}
	font = push_font;
	fore = push_fore;
}

static void paint_layers() {
	paint_background();
	paint_automap();
	paint_overlays();
	paint_points(red_markers, red_marker);
	if(show_party)
		paint_party_position();
}

static void input_automap() {
	switch(hot.key) {
	case KeyEscape:
	case KeySpace: breakmodal(0); break;
	}
}

void show_automap(const pointca& markers, int explore_radius) {
	show_fog_of_war = true;
	show_secrets = false;
	show_party = true;
	red_markers = 0;
	paint_layers();
	draw::screenshoot before;
	for(auto v : markers)
		loc->set(v, CellExplored, explore_radius);
	red_markers = &markers;
	paint_layers();
	screenshoot after;
	before.blend(after, 1000);
	show_scene(paint_layers, input_automap, 0);
}

void show_automap(bool mshow_fog_of_war, bool mshow_secrets, bool mshow_party, const pointca* vred_markers) {
	show_fog_of_war = mshow_fog_of_war;
	show_secrets = mshow_secrets;
	show_party = mshow_party;
	red_markers = vred_markers;
	show_scene(paint_layers, input_automap, 0);
}

void show_dungeon_automap() {
	show_automap(true, false, true, 0);
}