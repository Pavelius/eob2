#include "creature.h"
#include "direction.h"
#include "draw.h"
#include "dungeon.h"
#include "cell.h"
#include "monster.h"
#include "party.h"
#include "rand.h"
#include "resid.h"
#include "timer.h"
#include "view.h"

using namespace draw;

const int distance_per_level = 4;
const int mpg = 8;

namespace {
struct palspr : pma {
	unsigned char		data[18][16];
};
struct renderi : point {
	short				z;
	short				frame[4];
	short unsigned		flags[4];
	const sprite*		rdata;
	creaturei*			target;
	unsigned char		pallette;
	short				percent;
	unsigned char		alpha;
	unsigned char		zorder;
	rect				clip;
	void				paint() const;
	void clear() { memset(this, 0, sizeof(renderi)); }
};
};
BSDATAC(renderi, 512)
static celln render_mirror1, render_mirror2;
static resid render_door_type, render_dungeon;
static int render_flipped_wall;
static sprite* map_tiles;
static pointc indecies[18];
static draw::surface scaler(320, 200, 32), scaler2(320, 200, 32);

namespace colors {
color damage(255, 255, 255);
color fire(255, 0, 0);
}

// Central indecies 3, 9, 13, 16
static char	pos_levels[18] = {
	3, 3, 3, 3, 3, 3, 3,
	2, 2, 2, 2, 2,
	1, 1, 1,
	0, 0, 0
};
static char walls_front[18] = {
	7, 7, 7, 7, 7, 7, 7,
	8, 8, 8, 8, 8,
	9, 9, 9,
	0, 0, 0,
};
static int wall_sizes[18] = {
	48, 48, 48, 48, 48, 48, 48,
	80, 80, 80, 80, 80,
	128, 128, 128,
	176, 176, 176
};
static short item_distances[][2] = {
	{1000, 0},
	{1000, 0},
	{100 * 1000 / 120, 0},
	{80 * 1000 / 120, 0},
	{64 * 1000 / 120, 64},
	{48 * 1000 / 120, 64},
	{40 * 1000 / 120, 128},
	{32 * 1000 / 120, 128},
};
static point item_position[18 * 4] = {
	{-16, 56}, {0, 56}, {-42, 60}, {-22, 60},
	{16, 56}, {32, 56}, {-2, 60}, {18, 60},
	{48, 56}, {64, 56}, {38, 60}, {58, 60},
	{80, 56}, {96, 56}, {78, 60}, {98, 60},
	{112, 56}, {128, 56}, {118, 60}, {138, 60},
	{144, 56}, {160, 56}, {158, 60}, {178, 60},
	{176, 56}, {192, 56}, {198, 60}, {218, 60},
	// Level 2
	{-29, 66}, {-3, 66}, {-65, 74}, {-31, 74},
	{23, 66}, {49, 66}, {3, 74}, {37, 74},
	{75, 66}, {101, 66}, {71, 74}, {105, 74},
	{127, 66}, {153, 66}, {139, 74}, {173, 74},
	{179, 66}, {205, 66}, {207, 74}, {241, 74},
	// Level 1
	{-27, 86}, {19, 86}, {-57, 98}, {1, 98},
	{65, 86}, {111, 86}, {59, 98}, {117, 98},
	{157, 86}, {203, 86}, {175, 98}, {233, 98},
	// Level 0
	{-107, 118}, {-29, 118}, {-152, 136}, {-56, 136},
	{49, 118}, {127, 118}, {40, 136}, {136, 136},
	{205, 118}, {283, 118}, {232, 136}, {328, 136},
};
point wall_position[18] = {
	{scrx / 2 - 48 * 3, scry / 2},
	{scrx / 2 - 48 * 2, scry / 2},
	{scrx / 2 - 48 * 1, scry / 2},
	{scrx / 2, scry / 2},
	{scrx / 2 + 48 * 1, scry / 2},
	{scrx / 2 + 48 * 2, scry / 2},
	{scrx / 2 + 48 * 3, scry / 2},
	// Level 2 (index 7)
	{scrx / 2 - 80 * 2, scry / 2},
	{scrx / 2 - 80 * 1, scry / 2},
	{scrx / 2, scry / 2},
	{scrx / 2 + 80 * 1, scry / 2},
	{scrx / 2 + 80 * 2, scry / 2},
	// Level 1 (index 12)
	{scrx / 2 - 128, scry / 2},
	{scrx / 2, scry / 2},
	{scrx / 2 + 128, scry / 2},
	// Level 0 (index 15)
	{scrx / 2, scry / 2},
	{scrx / 2, scry / 2},
	{scrx / 2, scry / 2},
};

static directions get_view_direction(directions d, directions d1) {
	static directions result[][5] = {
		{Center, Left, Up, Right, Down},
		{Center, Up, Right, Down, Left},
		{Center, Left, Up, Right, Down},
		{Center, Down, Left, Up, Right},
		{Center, Right, Down, Left, Up},
	};
	return result[d][d1];
}

static celln get_wall_type(celln v) {
	switch(v) {
	case CellSecretButton:
	case CellPortal:
	case CellStairsUp:
	case CellStairsDown:
	case CellWall:
		return CellWall;
	default:
		return v;
	}
}

static int get_tile(celln id, bool mirrored) {
	if(id == CellWall && mirrored)
		return 1 * walls_frames;
	return bsdata<celli>::elements[id].frame;
}

static int get_tile_alternate(celln id) {
	return decor_offset + 2 * decor_frames;
}

renderi* get_disp(const void* target) {
	if(!target)
		return 0;
	for(auto& e : bsdata<renderi>()) {
		if(e.target == target)
			return &e;
	}
	return 0;
}

static unsigned flip_flags(pointc pos, directions direction) {
	return ((pos.x + pos.y + direction) & 1) != 0 ? ImageMirrorH : 0;
}

static void fast_shadow(unsigned char* d, int d_scan, int width, int height, unsigned char alpha) {
	if(!alpha)
		return;
	while(height-- > 0) {
		color* d1 = (color*)d;
		color* d2 = d1 + width;
		switch(alpha) {
		case 192:
			while(d1 < d2) {
				if(d1->a != 0xFF) {
					d1->r >>= 3;
					d1->g >>= 3;
					d1->b >>= 3;
				}
				d1++;
			}
			break;
		case 128:
			while(d1 < d2) {
				if(d1->a != 0xFF) {
					d1->r >>= 2;
					d1->g >>= 2;
					d1->b >>= 2;
				}
				d1++;
			}
			break;
		case 64:
			while(d1 < d2) {
				if(d1->a != 0xFF) {
					d1->r >>= 1;
					d1->g >>= 1;
					d1->b >>= 1;
				}
				d1++;
			}
			break;
		}
		d += d_scan;
	}
}

static void fast_fill_contour(unsigned char* d, int d_scan, int width, int height, color c1) {
	while(height-- > 0) {
		color* d1 = (color*)d;
		color* d2 = d1 + width;
		while(d1 < d2) {
			if(d1->a != 0xFF)
				*d1 = c1;
			d1++;
		}
		d += d_scan;
	}
}

void view_dungeon_reset() {
	bsdata<renderi>::source.clear();
	map_tiles = 0;
}

void set_dungeon_tiles(resid type) {
	map_tiles = gres(type);
	render_flipped_wall = 1 * walls_frames;
	render_dungeon = type;
	render_door_type = BRICK;
	render_mirror1 = CellSecretButton;
	render_mirror2 = CellPassable;
	switch(type) {
	case BLUE:
		render_door_type = BLUE;
		break;
	case DROW:
		render_door_type = DROW;
		break;
	case GREEN:
		render_door_type = GREEN;
		break;
	case BRICK:
		render_mirror1 = CellPassable;
		break;
	default:
		break;
	}
}

static renderi* add_render() {
	auto p = bsdata<renderi>::add();
	p->clear();
	return p;
}

static renderi* add_copy_render() {
	if(!bsdata<renderi>::source)
		return bsdata<renderi>::add();
	auto p1 = bsdata<renderi>::elements + (bsdata<renderi>::source.count - 1);
	auto p = add_render();
	memcpy(p, p1, sizeof(renderi));
	return p;
}

static dungeoni::overlayi* add_wall_decor(renderi* p, pointc index, directions dir, int n, bool flip, bool use_flip) {
	if(n == -1)
		return 0;
	auto bd = to(party.d, dir);
	auto index_start = to(index, bd);
	if(!index_start)
		return 0;
	auto t1 = loc->get(index_start);
	if(t1 == CellWall || t1 == CellStairsUp || t1 == CellStairsDown || t1 == CellPortal || t1 == CellDoor)
		return 0;
	auto povr = loc->get(index_start, to(bd, Down));
	if(!povr)
		return 0;
	auto tile = povr->type;
	if(tile < CellPuller)
		return 0;
	auto frame = get_tile(tile, false);
	if(frame == -1)
		return 0;
	if(tile == CellPuller) {
		if(povr->is(CellActive))
			frame += decor_frames;
	}
	p->frame[1] = frame + n;
	if(use_flip && flip && (render_mirror1 == tile || render_mirror2 == tile))
		p->flags[1] ^= ImageMirrorH;
	return povr;
}

static void fill_item_sprite(renderi* p, const itemi* pi, int frame = 0) {
	if(pi->is(Small))
		p->rdata = gres(ITEMGS);
	else
		p->rdata = gres(ITEMGL);
	p->frame[frame] = pi->avatar_ground;
}

static void set_percent(renderi* p, int d) {
	p->percent = item_distances[d][0];
	if(p->percent == 1000)
		p->percent = 0;
}

static void add_cellar_items(int i, dungeoni::overlayi* povr) {
	if(!povr)
		return;
	if(povr->type == CellCellar) {
		const int dy1 = 8;
		const int dy2 = 16;
		const int dy3 = 17;
		static point positions[18] = {{scrx / 2 - 48 * 3, scry / 2},
			{scrx / 2 - 48 * 2, scry / 2 - dy3},
			{scrx / 2 - 48 * 1, scry / 2 - dy3},
			{scrx / 2, scry / 2 - dy3},
			{scrx / 2 + 48 * 1, scry / 2 - dy3},
			{scrx / 2 + 48 * 2, scry / 2 - dy3},
			{scrx / 2 + 48 * 3, scry / 2 - dy3},
			// Level 2
			{scrx / 2 - 80 * 2, scry / 2 - dy2},
			{scrx / 2 - 80 * 1, scry / 2 - dy2},
			{scrx / 2, scry / 2 - dy2},
			{scrx / 2 + 80 * 1, scry / 2 - dy2},
			{scrx / 2 + 80 * 2, scry / 2 - dy2},
			// Level 1
			{scrx / 2 - 128, scry / 2 - dy1},
			{scrx / 2, scry / 2 - dy1},
			{scrx / 2 + 128, scry / 2 - dy1},
			// Level 0
			{scrx / 2, scry / 2},
			{scrx / 2, scry / 2},
			{scrx / 2, scry / 2},
		};
		item* result[4];
		auto item_count = loc->getitems(result, lenghtof(result), povr);
		auto d = pos_levels[i] * 2;
		for(size_t n = 0; n < item_count; n++) {
			auto p = add_render();
			p->x = positions[i].x + (short)n;
			p->y = positions[i].y + (short)(n / 2);
			p->z = pos_levels[i] * distance_per_level;
			p->zorder = 2;
			p->alpha = (unsigned char)item_distances[d][1];
			set_percent(p, d);
			fill_item_sprite(p, &result[n]->geti());
		}
	}
}

static void create_wall(int i, pointc index, int frame, celln rec, bool flip) {
	int n;
	// Walls render
	// | |_  5 4 7
	// | |_  6 3 8
	//   |_    2 9
	//   |     1
	// Wall width: 48, 80, 128, Wall height: 37, 59, 96
	static int wall_width[18] = {48, 48, 48, 48, 48, 48, 48,
		80, 80, 80, 80, 80,
		128, 128, 128,
		0, 0, 0,
	};
	static char walls_left[18] = {0, 5, 4, 0, 0, 0, 0,
		6, 3, 0, 0, 0,
		2, 0, 0,
		1, 0, 0,
	};
	static char walls_right[18] = {0, 0, 0, 0, 4, 5, 0,
		0, 0, 0, 3, 6,
		0, 0, 2,
		0, 0, 1,
	};
	// Decors render
	// 9 7 3 7 9
	// 8 6 2 6 8
	// 8 5 1 5 8
	//   4 0 4
	//     ^ - party pos.
	static char decor_front[18] = {
		3, 3, 3, 3, 3, 3, 3,
		2, 2, 2, 2, 2,
		1, 1, 1,
		0, 0, 0,
	};
	static char decor_right[18] = {
		-1, -1, -1, -1, 7, 9, 9,
		-1, -1, -1, 6, 8,
		-1, -1, 5,
		-1, -1, 4,
	};
	static char decor_left[18] = {
		9, 9, 7, -1, -1, -1, -1,
		8, 6, -1, -1, -1,
		5, -1, -1,
		4, -1, -1,
	};
	bool enable;
	auto cd = party.d;
	if(frame == -1)
		return;
	// Front
	n = walls_front[i];
	if(n) {
		auto p = add_render();
		p->x = wall_position[i].x;
		p->y = scry / 2;
		p->z = pos_levels[i] * distance_per_level;
		p->frame[0] = n + frame;
		p->rdata = map_tiles;
		auto front_wall = p;
		if(rec == CellDoor && i < 15) {
			p = add_render();
			p->x = wall_position[i].x;
			p->y = scry / 2;
			p->z = pos_levels[i] * distance_per_level;
			p->zorder = 1;
			p->rdata = map_tiles;
			auto& e1 = map_tiles->get(door_offset + pos_levels[i] - 1);
			auto& e2 = map_tiles->get(door_offset + 6 + pos_levels[i] - 1);
			auto po = loc->get(to(indecies[i], to(party.d, Down)), party.d);
			switch(render_door_type) {
			case BLUE:
				if(loc->is(index, CellActive)) {
					if(po) {
						auto w = e1.sx - (e2.sx * 3) / 2; p->x -= w;
						p->zorder = 2;
						p->frame[0] = door_offset + 6 + pos_levels[i] - 1;
						p = add_copy_render();
						p->x += 2 * w;
						p->flags[0] = ImageMirrorH;
					} else
						bsdata<renderi>::source.remove(); // Remove door
				} else {
					auto p1 = p;
					p->frame[0] = door_offset + pos_levels[i] - 1;
					p = add_copy_render();
					p->flags[0] = ImageMirrorH;
					p->frame[0] = door_offset + pos_levels[i] - 1;
					if(po) {
						p1[0].frame[1] = door_offset + 6 + pos_levels[i] - 1;
						p1[1].frame[1] = door_offset + 6 + pos_levels[i] - 1;
						p1[1].flags[1] = ImageMirrorH;
					}
				}
				break;
			case DROW:
				p->frame[0] = door_offset + pos_levels[i] - 1;
				if(loc->is(index, CellActive)) {
					auto x = wall_position[i].x - e1.ox;
					auto y = wall_position[i].y - e1.oy;
					p->y = p->y - e1.sy + e1.sy / 10;
					p->clip.set(x, y, x + e1.sx, y + e1.sy);
				}
				front_wall->frame[1] = door_offset + pos_levels[i] - 1 + 3;
				if(po)
					front_wall->frame[2] = door_offset + 6 + pos_levels[i] - 1;
				break;
			case GREEN:
				if(loc->is(index, CellActive))
					bsdata<renderi>::source.remove(); // Remove door
				else
					p->frame[0] = door_offset + pos_levels[i] - 1;
				if(po)
					front_wall->frame[1] = door_offset + 6 + pos_levels[i] - 1;
				break;
			default:
				// Drop down door
				p->frame[0] = door_offset + pos_levels[i] - 1;
				//if(loc->type == CRIMSON) {
				//	p->x++;
				//	p->y++;
				//}
				if(loc->is(index, CellActive)) {
					auto x = wall_position[i].x - e1.ox;
					auto y = wall_position[i].y - e1.oy;
					p->y = p->y - e1.sy + e1.sy / 10;
					p->clip.set(x, y, x + e1.sx, y + e1.sy);
				}
				if(po)
					front_wall->frame[1] = door_offset + 6 + pos_levels[i] - 1;
				break;
			}
		} else {
			if(i == 13) {
				auto t1 = to(index, to(cd, Down));
				if(loc->get(t1) == CellDoor) {
					p->frame[1] = decor_offset + 1;
					if(flip)
						p->flags[1] = ImageMirrorH;
				}
			}
			switch(loc->type) {
			case BRICK:
				if(rec == CellStairsUp)
					p->frame[1] = decor_offset + 19 * decor_frames + pos_levels[i];
				else if(rec == CellStairsDown)
					p->frame[1] = decor_offset + 20 * decor_frames + pos_levels[i];
				break;
			case FOREST:
				if(rec == CellStairsUp || rec == CellStairsDown) {
					p->frame[0] = walls_front[i];
					p->frame[1] = decor_offset + 0 * decor_frames + pos_levels[i];
				}
				break;
			default:
				break;
			}
			auto povr = add_wall_decor(p, index, Down, decor_front[i], flip, true);
			add_cellar_items(i, povr);
		}
	}
	// Left
	n = walls_left[i];
	enable = true;
	if((rec == CellStairsUp || rec == CellStairsDown) && i != 15)
		enable = false;
	if(n && enable) {
		auto p = add_render();
		p->x = wall_position[i].x;
		if(n == 5 || n == 6)
			p->x += wall_width[i] * 2;
		else
			p->x += wall_width[i];
		p->y = scry / 2;
		p->z = pos_levels[i] * distance_per_level + 2;
		p->frame[0] = n + frame;
		p->rdata = map_tiles;
		add_wall_decor(p, index, Right, decor_left[i], flip, false);
	}
	// Right
	n = walls_right[i];
	enable = true;
	if((rec == CellStairsUp || rec == CellStairsDown) && i != 17)
		enable = false;
	if(n && enable) {
		auto p = add_render();
		p->x = wall_position[i].x;
		if(n == 5 || n == 6)
			p->x -= wall_width[i] * 2;
		else
			p->x -= wall_width[i];
		p->y = scry / 2;
		p->z = pos_levels[i] * distance_per_level + 2;
		p->flags[0] = ImageMirrorH;
		p->frame[0] = n + frame;
		p->rdata = map_tiles;
		p->flags[1] = ImageMirrorH;
		add_wall_decor(p, index, Left, decor_right[i], flip, false);
	}
}

static bool is_between_vert_walls(pointc index) {
	return get_wall_type(loc->get(to(index, party.d))) == CellWall
		&& get_wall_type(loc->get(to(index, to(party.d, Down)))) == CellWall;
}

static void set_back_position(point& result, int i, resid type) {
	const int w1 = 30, w2 = 56, w3 = 84;
	const int h1 = 28, h2 = 32, h3 = 40;
	static point back_position[18] = {{scrx / 2 - w1 / 2 - w1 * 3, scry / 2 - h1},
		{scrx / 2 - w1 / 2 - w1 * 2, scry / 2 - h1},
		{scrx / 2 - w1 / 2 - w1 * 1, scry / 2 - h1},
		{scrx / 2 - w1 / 2, scry / 2 - h1},
		{scrx / 2 - w1 / 2 + w1 * 1, scry / 2 - h1},
		{scrx / 2 - w1 / 2 + w1 * 2, scry / 2 - h1},
		{scrx / 2 - w1 / 2 + w1 * 3, scry / 2 - h1},
		// Level 2
		{scrx / 2 - w2 / 2 - w2 * 2, scry / 2 - h2},
		{scrx / 2 - w2 / 2 - w2 * 1, scry / 2 - h2},
		{scrx / 2 - w2 / 2, scry / 2 - h2},
		{scrx / 2 - w2 / 2 + w2 * 1, scry / 2 - h2},
		{scrx / 2 - w2 / 2 + w2 * 2, scry / 2 - h2},
		// Level 1
		{scrx / 2 - w3 / 2 - w3, scry / 2 - h3},
		{scrx / 2 - w3 / 2, scry / 2 - h3},
		{scrx / 2 - w3 / 2 + w3, scry / 2 - h3},
		// Level 0
		{scrx / 2, scry / 2},
		{scrx / 2, scry / 2},
		{scrx / 2, scry / 2},
	};
	result.x = back_position[i].x;
	result.y = back_position[i].y;
	switch(type) {
	case DROW:
		switch(pos_levels[i]) {
		case 1: result.y += 5; break;
		case 2: result.y += 3; break;
		case 3: result.y += 2; break;
		}
		break;
	case XANATHA:
		switch(pos_levels[i]) {
		case 1: result.y += 2; break;
		}
		break;
   default:
      break;
	}
}

static void create_overlay(int i, pointc index, int frame, bool mr) {
	static int wall_percent[18] = {
		375, 375, 375, 375, 375, 375, 375,
		625, 625, 625, 625, 625,
		0, 0, 0,
		0, 0, 0,
	};
	static unsigned char wall_alpha[18] = {
		128, 128, 128, 128, 128, 128, 128,
		64, 64, 64, 64, 64,
		0, 0, 0,
		0, 0, 0,
	};
	if(i >= 15)
		return;
	if(is_between_vert_walls(index))
		return; // Return if not front look at
	auto n = loc->textures[frame];
	if(n == 0xFFFF)
		return;
	// Background
	auto p = add_render();
	set_back_position(*p, i, loc->type);
	p->percent = wall_percent[i];
	p->alpha = wall_alpha[i];
	p->z = pos_levels[i] * distance_per_level + 2;
	p->frame[0] = n;
	p->rdata = gres(OVERLAYS);
	auto pb = p;
	// Door entrance
	p = add_render();
	p->x = wall_position[i].x;
	p->y = scry / 2;
	p->z = pos_levels[i] * distance_per_level;
	p->frame[0] = get_tile(CellDoor, mr) + walls_front[i];
	p->rdata = map_tiles;
	switch(loc->type) {
	case DROW:
		p->frame[1] = door_offset + pos_levels[i] - 1 + 3;
		switch(pos_levels[i]) {
		case 1: pb->y += 5; break;
		case 2: pb->y += 3; break;
		case 3: pb->y += 2; break;
		}
		break;
	case XANATHA:
		switch(pos_levels[i]) {
		case 1: pb->y += 2; break;
		}
		break;
   default:
      break;
	}
}

static bool cat_by_walls(int i1, int i2) {
	return get_wall_type(loc->get(indecies[i1])) == CellWall
		&& get_wall_type(loc->get(indecies[i2])) == CellWall;
}

static bool cat_by_walls(int i) {
	switch(i) {
	case 2: return cat_by_walls(3, 8);
	case 4: return cat_by_walls(3, 10);
	case 8: return cat_by_walls(9, 12);
	case 9: return get_wall_type(loc->get(indecies[13])) == CellWall;
	case 10: return cat_by_walls(9, 14);
	case 12: return cat_by_walls(13, 15);
	case 14: return cat_by_walls(13, 17);
	default: return false;
	}
}

static void create_floor(int i, pointc index, celln rec, bool flip) {
	static short floor_pos[18] = {
		scrx / 2 - 42 * 3, scrx / 2 - 42 * 2, scrx / 2 - 42, scrx / 2, scrx / 2 + 42, scrx / 2 + 42 * 2, scrx / 2 + 42 * 3,
		scrx / 2 - 64 * 2, scrx / 2 - 64, scrx / 2, scrx / 2 + 64, scrx / 2 + 64 * 2,
		scrx / 2 - 98, scrx / 2, scrx / 2 + 98,
		scrx / 2 - 176, scrx / 2, scrx / 2 + 176,
	};
	static char floor_frame[18] = {3, 3, 3, 3, 3, 3, 3,
		2, 2, 2, 2, 2,
		1, 1, 1,
		0, 0, 0,
	};
	auto frame = get_tile(rec, false);
	if(frame != -1) {
		auto& ei = bsdata<celli>::elements[rec];
		auto p = add_render();
		p->x = floor_pos[i];
		p->y = scry / 2;
		p->z = pos_levels[i] * distance_per_level;
		if(ei.flags.is(FloorLevel))
			p->z += 2;
		if(flip)
			p->flags[0] = ImageMirrorH;
		if(rec == CellButton && loc->is(index, CellActive))
			frame = get_tile_alternate(rec);
		p->frame[0] = floor_frame[i] + frame;
		if(ei.res)
			p->rdata = gres(ei.res);
		else
			p->rdata = map_tiles;
	}
}

static int get_x_from_line(int y, int x1, int y1, int x2, int y2) {
	return ((y - y1) * (x2 - x1)) / (y2 - y1) + x1;
}

static int cell_position(int i, int side) {
	return pos_levels[i] * 2 + ((side < 2) ? 1 : 0);
}

static int monster_position(int i, int side) {
	return pos_levels[i] * 2 + ((side < 2) ? 0 : -1);
}

static void create_items(int i, pointc v, directions dr) {
	dungeoni::ground* result[64];
	auto item_count = loc->getitems(result, lenghtof(result), v);
	for(size_t n = 0; n < item_count; n++) {
		auto pi = (dungeoni::ground*)result[n];
		auto s = get_side(pi->side, dr);
		auto d = cell_position(i, s);
		auto p = add_render();
		p->x = item_position[i * 4 + s].x;
		p->y = item_position[i * 4 + s].y;
		p->z = pos_levels[i] * distance_per_level + 1 + (1 - s / 2);
		p->alpha = (unsigned char)item_distances[d][1];
		// p->zorder = 1;
		set_percent(p, d);
		fill_item_sprite(p, &pi->geti());
	}
}

void create_monster_pallette() {
	auto pm = player->getmonster();
	if(!pm)
		return;
	auto pr = gres(pm->res);
	if(!pr)
		return;
	auto ph = pr->getheader("COL");
	if(!ph)
		return;
	player->pallette = xrand(0, 2);
}

static void create_monsters(int i, pointc index, directions dr, bool flip) {
	if(cat_by_walls(i))
		return;
	if(!pos_levels[i])
		return;
	creaturei* result[6]; loc->getmonsters(result, index, dr);
	for(int n = 0; n < 4; n++) {
		auto pc = result[n];
		if(!pc)
			continue;
		auto large = pc->is(Large);
		auto dir = get_view_direction(dr, pc->d);
		auto d = monster_position(i, n);
		auto p = add_render();
		if(large) {
			p->x = item_position[i * 4 + 0].x + (item_position[i * 4 + 1].x - item_position[i * 4 + 0].x) / 2;
			p->y = item_position[i * 4 + 0].y + (item_position[i * 4 + 3].y - item_position[i * 4 + 0].y) / 2;
			p->z = pos_levels[i] * distance_per_level - 1;
			d = pos_levels[i] * 2 - 1;
		} else {
			p->x = item_position[i * 4 + n].x;
			p->y = item_position[i * 4 + n].y;
			p->z = pos_levels[i] * distance_per_level + 1 - (n / 2);
		}
		set_percent(p, d);
		p->alpha = (unsigned char)item_distances[d][1];
		auto pm = pc->getmonster();
		if(pm)
			p->rdata = gres(pm->res);
		if(!p->rdata)
			continue;
		p->target = pc;
		p->pallette = pc->pallette;
		// p->zorder = 2;
		unsigned flags = 0;
		// Animate active monsters
		if(((p->x + current_cpu_time / 100) / 16) % 2) {
			p->x++;
			p->y++;
		}
		switch(dir) {
		case Left:
			pc->setframe(p->frame, flip ? 2 : 1);
			break;
		case Right:
			pc->setframe(p->frame, flip ? 2 : 1);
			flags |= ImageMirrorH;
			break;
		case Up:
			pc->setframe(p->frame, 3);
			if(flip)
				flags ^= ImageMirrorH;
			break;
		case Down:
			pc->setframe(p->frame, 0);
			if(flip)
				flags ^= ImageMirrorH;
			break;
		}
		for(int i = 0; i < lenghtof(p->flags); i++)
			p->flags[i] = flags;
	}
}

static void prepare_draw(pointc index, directions dr) {
	static char offset_north[18][2] = {{-3, -3}, {-2, -3}, {-1, -3}, {0, -3}, {1, -3}, {2, -3}, {3, -3},
		{-2, -2}, {-1, -2}, {0, -2}, {1, -2}, {2, -2},
		{-1, -1}, {0, -1}, {1, -1},
		{-1, 0}, {0, 0}, {1, 0}
	};
	static char offset_west[18][2] = {{-3, 3}, {-3, 2}, {-3, 1}, {-3, 0}, {-3, -1}, {-3, -2}, {-3, -3},
		{-2, 2}, {-2, 1}, {-2, 0}, {-2, -1}, {-2, -2},
		{-1, 1}, {-1, 0}, {-1, -1},
		{0, 1}, {0, 0}, {0, -1}
	};
	static char offset_south[18][2] = {{3, 3}, {2, 3}, {1, 3}, {0, 3}, {-1, 3}, {-2, 3}, {-3, 3},
		{2, 2}, {1, 2}, {0, 2}, {-1, 2}, {-2, 2},
		{1, 1}, {0, 1}, {-1, 1},
		{1, 0}, {0, 0}, {-1, 0}
	};
	static char offset_east[18][2] = {{3, -3}, {3, -2}, {3, -1}, {3, 0}, {3, 1}, {3, 2}, {3, 3},
		{2, -2}, {2, -1}, {2, 0}, {2, 1}, {2, 2},
		{1, -1}, {1, 0}, {1, 1},
		{0, -1}, {0, 0}, {0, 1}
	};
	int x = index.x;
	int y = index.y;
	char *offsets;
	switch(dr) {
	case Up:
		offsets = (char*)offset_north;
		break;
	case Down:
		offsets = (char*)offset_south;
		break;
	case Left:
		offsets = (char*)offset_west;
		break;
	case Right:
		offsets = (char*)offset_east;
		break;
	default:
		return;
	}
	bsdata<renderi>::source.clear();
	// indeciess
	for(int i = 0; i < 18; i++) {
		indecies[i].x = x + offsets[i * 2 + 0];
		indecies[i].y = y + offsets[i * 2 + 1];
	}
	// walls
	for(int i = 0; i < 18; i++) {
		pointc index = indecies[i];
		bool mr = ((index.x + index.y + party.d) & 1) != 0;
		if(!index) {
			create_wall(i, {-1, -1}, get_tile(CellWall, mr), CellWall, !mr);
			continue;
		}
		auto tile = loc->get(index);
		auto tilt = get_wall_type(tile);
		if(tilt != CellWall) {
			if(tilt != CellDoor) {
				if(locup && locup->get(index) == CellPit)
					create_floor(i, index, CellPitUp, mr);
			}
			create_items(i, index, dr);
			create_monsters(i, index, dr, mr);
		}
		auto& et = bsdata<celli>::elements[tile];
		if(et.flags.is(LookWall)) {
			if(et.res == OVERLAYS)
				create_overlay(i, index, et.frame, mr);
			else
				create_wall(i, index, get_tile(tile, mr), tile, mr);
		} else if(et.flags.is(LookObject))
			create_floor(i, index, tile, mr);
	}
}

static int compare_drawable(const void* p1, const void* p2) {
	renderi* e1 = *((renderi**)p1);
	renderi* e2 = *((renderi**)p2);
	if(e1->z != e2->z)
		return e2->z - e1->z;
	else if(e1->zorder != e2->zorder)
		return e1->zorder - e2->zorder;
	else if(e1->y != e2->y)
		return e1->y - e2->y;
	else if(e1->x != e2->x)
		return e1->x - e2->x;
	return e2 - e1;
}

static void imagex(int x, int y, const sprite* res, int id, unsigned flags, int percent, unsigned char shadow) {
	if(!percent)
		percent = 1000;
	const sprite::frame& f = res->get(id);
	int sx = f.sx;
	int sy = f.sy;
	int ssx = f.sx * percent / 1000;
	int ssy = f.sy * percent / 1000;
	int sox = f.ox * percent / 1000;
	int soy = f.oy * percent / 1000;
	unsigned flags_addon = (flags & (ImagePallette));
	if(true) {
		rectpush push;
		auto push_canvas = canvas;
		auto push_fore = fore;
		draw::canvas = &scaler;
		draw::fore.r = draw::fore.g = draw::fore.b = 0; draw::fore.a = 0xFF;
		caret.x = 0; caret.y = 0; width = sx; height = sy;
		rectf();
		if(flags & ImageMirrorH)
			draw::image(sx, 0, res, id, ImageMirrorH | ImageNoOffset | flags_addon);
		else
			draw::image(0, 0, res, id, ImageNoOffset | flags_addon);
		fore = push_fore;
		canvas = push_canvas;
	}
	blit(scaler2, 0, 0, ssx, ssy, 0, scaler, 0, 0, sx, sy);
	fast_shadow(scaler2.bits, scaler2.scanline, ssx, ssy, shadow);
	if(flags & ImageColor)
		fast_fill_contour(scaler2.bits, scaler2.scanline, ssx, ssy, colors::white);
	if(flags & ImageMirrorH)
		blit(*draw::canvas, x - ssx + sox, y - soy, ssx, ssy, ImageTransparent, scaler2, 0, 0);
	else
		blit(*draw::canvas, x - sox, y - soy, ssx, ssy, ImageTransparent, scaler2, 0, 0);
}

void fix_monster_damage(const creaturei* target) {
	auto p = get_disp(target);
	if(!p)
		return;
	need_update_animation = true;
	for(auto& e : p->flags)
		e |= ImageColor;
}

void fix_monster_damage_end() {
	for(auto& e : bsdata<renderi>()) {
		if(loc->have((creaturei*)e.target)) {
			for(auto& f : e.flags)
				f &= ~ImageColor;
		}
	}
}

void fix_monster_attack(const creaturei* target) {
	auto p = get_disp(target);
	if(p) {
		need_update_animation = true;
		target->setframe(p->frame, 4);
		fix_animate();
		target->setframe(p->frame, 5);
		need_update_animation = true;
	}
}

void fix_monster_attack_end(const creaturei* target) {
	auto p = get_disp(target);
	if(p) {
		fix_animate();
		target->setframe(p->frame, 0);
	}
}

void add_color_data(pma* result, const unsigned char* bitdata) {
	palspr* epr = static_cast<palspr*>(result);
	epr->name[0] = 'C';
	epr->name[1] = 'O';
	epr->name[2] = 'L';
	epr->name[3] = 0;
	epr->size = sizeof(*epr);
	epr->count = 18;
	for(auto x = 0; x < 18; x++) {
		for(auto y = 0; y < 16; y++)
			epr->data[x][y] = bitdata[(200 - 16 + y) * 320 + (320 - 18 + x)];
	}
}

void renderi::paint() const {
	color pal[256];
	auto push_pal = palt;
	unsigned flags_addon = 0;
	if(pallette) {
		auto s1 = (palspr*)rdata->getheader("COL");
		if(s1) {
			auto& fr = rdata->get(0);
			auto pa = (color*)rdata->ptr(fr.pallette);
			memcpy(pal, pa, sizeof(pal));
			draw::palt = pal;
			for(auto i = 0; i < 16; i++) {
				auto i1 = s1->data[0][i];
				if(!i1)
					break;
				pal[i1] = pa[s1->data[pallette][i]];
			}
			flags_addon |= ImagePallette;
		}
	}
	for(int i = 0; i < 4; i++) {
		if(i && !frame[i])
			break;
		if(!percent && (flags[i] & ImageColor) == 0)
			image(x, y, rdata, frame[i], flags[i] | flags_addon);
		else
			imagex(x, y, rdata, frame[i], flags[i] | flags_addon, percent, alpha);
	}
	palt = push_pal;
}

void paint_dungeon() {
	// Points of view like this:
	//
	// A|B|C|D|E|F|G
	//   - - - - -
	//   H|I|J|K|L
	//     - - -
	//     M|N|O
	//     - - -
	//     P|^|Q
	//
	// '^' in the picture is the party position, facing north.
	// '|' and '-' in the above picture resembled walls.
	// There are a total of 25 different wall positions.
	// To render all the walls correctly 17 maze positions must be read(A-Q).
	//
	// Walls render
	// | |_  4 3 6
	// | |_  5 2 7
	//   |_    1 8
	//   |     0
	// Decors render
	// 9 7 3 7 9
	// 8 6 2 6 8
	// 8 5 1 5 8
	//   4 0 4
	//     ^ - party pos.
	// Size by levels:
	// Each size (S) covert front (F) left side (L) and right side (R) size of lower level.
	// So FS1 = LS2 + FS2 + RS2
	// 4 - 32x24, side has 8, only walls and items
	// 3 - 48x37, side has 16
	// 2 - 80x59, side has 24
	// 1 - 128x96, side has 24
	// 0 - 176x120 - background
	renderi* zorder[512];
	auto push_clip = clipping;
	unsigned flags = flip_flags(party, party.d);
	image(scrx / 2, scry / 2, map_tiles, 0, flags);
	setclip({0, 0, scrx - 1, scry - 1});
	renderi** pz = zorder;
	for(auto& e : bsdata<renderi>())
		*pz++ = &e;
	qsort(zorder, pz - zorder, sizeof(zorder[0]), compare_drawable);
	for(auto p1 = zorder; p1 < pz; p1++) {
		renderi* p = *p1;
		if(p->clip) {
			auto push_clip = clipping;
			setclip(p->clip);
			p->paint();
			clipping = push_clip;
		} else
			p->paint();
	}
	clipping = push_clip;
}

void animation_update() {
	if(loc)
		prepare_draw(party, party.d);
}

renderi* get_last_disp() {
	for(auto& e : bsdata<renderi>()) {
		if(!e.rdata)
			return &e;
	}
	return 0;
}

static int get_index_pos(pointc index) {
	for(int i = 0; i < 18; i++) {
		if(indecies[i] == index)
			return i;
	}
	return -1;
}

int thrown_side(int avatar_thrown, int side) {
	if((avatar_thrown >= 2 && avatar_thrown < 7)
		|| (avatar_thrown >= 200))
		return -1;
	return side;
}

static void fill_sprite(renderi* p, int avatar_thrown, directions drs, int side) {
	if(avatar_thrown >= 200) {
		p->frame[0] = avatar_thrown - 200;
		p->rdata = gres(ITEMGL);
	} else if(avatar_thrown >= 100) {
		p->frame[0] = avatar_thrown - 100;
		p->rdata = gres(ITEMGS);
	} else if(avatar_thrown >= 7) {
		p->frame[0] = avatar_thrown;
		p->rdata = gres(THROWN);
		if(drs == Down) {
			p->flags[0] |= ImageMirrorV;
			p->frame[0]++;
		}
	} else {
		p->frame[0] = avatar_thrown;
		p->rdata = gres(THROWN);
	}
}

static void fill_position(renderi* p, int i, int ps, int side) {
	int d = pos_levels[i] * 2 + (1 - ps / 2);
	switch(side) {
	case 0:
		p->y = 24 + d * 2;
		p->x = get_x_from_line(p->y, (176 - 72) / 2 + 14, 24, (176 - 32) / 2 + 6, 40);
		break;
	case 1:
		p->y = 24 + d * 2;
		p->x = get_x_from_line(p->y, (176 - 72) / 2 + 72 - 14, 24, (176 - 32) / 2 + 32 - 6, 40);
		break;
	default:
		p->x = 176 / 2;
		p->y = 40 + d;
		break;
	}
	p->z = pos_levels[i] * distance_per_level + (1 - ps / 2);
}

static renderi* create_thrown(renderi* p, int i, int ps, int avatar_thrown, directions dr, int side) {
	if(!p)
		p = add_render();
	p->clear();
	fill_position(p, i, ps, side);
	fill_sprite(p, avatar_thrown, dr, side);
	int d = cell_position(i, ps);
	p->percent = item_distances[d][0];
	p->alpha = (unsigned char)item_distances[d][1];
	return p;
}

static void thrown_step(pointc v, directions d, int avatar_thrown, int side) {
	int i = get_index_pos(v);
	if(i == -1)
		return;
	int s1, s2;
	switch(d) {
	case Up:
		s1 = 2;
		s2 = 0;
		break;
	default:
		s1 = 0;
		s2 = 2;
		break;
	}
	auto p = create_thrown(0, i, s1, avatar_thrown, d, side);
	paint_dungeon();
	doredraw();
	waitcputime(64);
	p = create_thrown(p, i, s2, avatar_thrown, d, side);
	paint_dungeon();
	doredraw();
	waitcputime(64);
	p->clear();
	bsdata<renderi>::source.remove(bsdata<renderi>::source.indexof(p), 1);
}

void thrown_item(pointc v, directions d, int avatar_thrown, int side, int distance) {
	if(!v)
		return;
	side = thrown_side(avatar_thrown, side);
	for(int i = 0; i < distance; i++) {
		thrown_step(v, d, avatar_thrown, side);
		v = to(v, to(party.d, d));
		if(!v)
			break;
	}
}

void show_dungeon_images() {
	static resid sprites[] = {BLUE, BRICK, CRIMSON, DROW, DUNG, GREEN, FOREST, MEZZ, SILVER, XANATHA};
	static int sprites_count = lenghtof(sprites);
	rectpush push;
	auto push_fore = fore;
	auto push_font = font;
	set_small_font();
	auto show_texture = true;
	auto po = gres(OVERLAYS);
	int focus = 0;
	int sprite_number = 0;
	while(ismodal()) {
		if(sprite_number < 0)
			sprite_number = 0;
		else if(sprite_number > sprites_count - 1)
			sprite_number = sprites_count - 1;
		auto pi = bsdata<residi>::elements + sprites[sprite_number];
		auto ps = pi->get();
		auto maximum = po->count;
		if(focus < 0)
			focus = 0;
		else if(focus > maximum - 1)
			focus = maximum - 1;
		paint_test_mode();
		image(scrx / 2, scry / 2, ps, 0, 0);
		auto push_clip = clipping; setclip({0, 0, scrx, scry});
		auto wall = bsdata<celli>::elements[CellWall].frame;
		caret = {scrx / 2, scry / 2};
		image(ps, 1 + wall + 3, 0);
		image(ps, 1 + wall + 3, ImageMirrorH);
		image(ps, 1 + wall + 2, 0);
		image(ps, 1 + wall + 2, ImageMirrorH);
		image(ps, 1 + wall + 1, 0);
		image(ps, 1 + wall + 1, ImageMirrorH);
		auto door = bsdata<celli>::elements[CellDoor].frame;
		auto dungeon = sprites[sprite_number];
		if(show_texture) {
			point position; set_back_position(position, 13, dungeon);
			image(position.x, position.y, po, focus, 0);
		}
		switch(dungeon) {
		case DROW:
			image(ps, 1 + door + 8, 0);
			image(ps, door_offset + 3, 0);
			break;
		default:
			image(ps, 1 + door + 8, 0);
			break;
		}
		caret.x -= 128; image(ps, 1 + wall + 8, 0);
		caret.x += 128 * 2; image(ps, 1 + wall + 8, 0);
		clipping = push_clip;
		// Footer
		caret = {5, 180};
		text(str("dungeon %1, texture %2i/%3i", pi->id, focus, maximum), -1, TextBold);
		domodal();
		switch(hot.key) {
		case KeyRight: focus++; break;
		case KeyLeft: focus--; break;
		case KeyDown: sprite_number++; break;
		case KeyUp: sprite_number--; break;
		case KeyEscape: breakmodal(0); break;
		case 'M': show_texture = !show_texture; break;
		}
	}
	font = push_font;
	fore = push_fore;
}
