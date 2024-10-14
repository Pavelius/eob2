#include "creature.h"
#include "direction.h"
#include "draw.h"
#include "dungeon.h"
#include "cell.h"
#include "party.h"
#include "resid.h"
#include "view.h"

using namespace draw;

namespace {
struct palspr : pma {
	unsigned char		data[18][16];
};
struct renderi {
	short				x, y, z;
	short				frame[4];
	const sprite*		rdata;
	pointc				pos;
	short unsigned		flags[4];
	creaturei*			pc;
	celln				rec;
	unsigned char		pallette;
	short				percent;
	unsigned char		alpha;
	unsigned char		zorder;
	rect				clip;
	void				paint() const;
	void clear() { memset(this, 0, sizeof(renderi)); }
};
}
static celln			render_mirror1, render_mirror2;
static resid			render_door_type, render_dungeon;
static int				render_flipped_wall;
static sprite*			map_tiles;
static int				disp_damage[6];
static int				disp_hits[6][2];
static renderi			disp_data[512];
const int				distance_per_level = 4;
const int				mpg = 8;
static pointc			indecies[18];
static draw::surface	scaler(320, 200, 32);
static draw::surface	scaler2(320, 200, 32);

namespace colors {
color damage(255, 255, 255);
color fire(255, 0, 0);
}

static char	pos_levels[18] = {
	3, 3, 3, 3, 3, 3, 3,
	2, 2, 2, 2, 2,
	1, 1, 1,
	0, 0, 0
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

static celln get_wall_type(celln v) {
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

static int get_tile(celln id, bool mirrored) {
	if(id == CellWall && mirrored)
		return 1 * walls_frames;
	return bsdata<celli>::elements[id].frame;
}

static inline int get_tile_alternate(celln id) {
	return decor_offset + 2 * decor_frames;
}

static renderi* get_monster_disp(creaturei* target) {
	if(!target)
		return 0;
	for(auto& e : disp_data) {
		if(!e.rdata)
			return 0;
		if(e.pc == target)
			return &e;
	}
	return 0;
}

static int get_party_index(const creaturei* target) {
	for(auto i = 0; i < 6; i++) {
		if(party.units[i] == target)
			return i;
	}
	return -1;
}

static int get_party_disp(creaturei* target, wearn id) {
	if(!target)
		return 0;
	int pind = get_party_index(target);
	if(pind == -1)
		return 0;
	if(id == RightHand)
		return disp_hits[pind][0];
	else if(id == LeftHand)
		return disp_hits[pind][1];
	return 0;
}

static unsigned flip_flags(pointc pos, directions direction) {
	return ((pos.x + pos.y + direction) & 1) != 0 ? ImageMirrorH : 0;
}

void fast_shadow(unsigned char* d, int d_scan, int width, int height, unsigned char alpha) {
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
	memset(disp_damage, 0, sizeof(disp_damage));
	memset(disp_data, 0, sizeof(disp_data));
	memset(disp_hits, 0, sizeof(disp_hits));
	map_tiles = 0;
}

static int get_dungeon_floor(pointc v, directions dir) {
	if(!v)
		return -1;
	int t = loc->get(v);
	if(t == CellButton)
		return loc->is(v, CellActive) ? 2 : 1;
	return -1;
}

static void render_player_damage(int x, int y, int hits, unsigned counter) {
	//draw::state push;
	//char temp[32]; stringbuilder sb(temp); sb.addint(hits);
	//draw::image(x, y - 1, gres(THROWN), 0, (counter % 2) ? ImageMirrorH : 0);
	//draw::fore = colors::damage;
	//draw::text(x - draw::textw(temp) / 2, y - 3, temp);
}

static void render_player_attack(int x, int y, int hits) {
	static int side;
	//draw::state push;
	//char temp[32]; stringbuilder sb(temp);
	//if(hits == -1)
	//	sb.add("miss");
	//else
	//	sb.add("%1i", hits);
	//draw::image(x, y - 1, gres(THROWN), 1, ((side++) % 2) ? ImageMirrorH : 0);
	//draw::fore = colors::damage;
	//draw::text(x - draw::textw(temp) / 2, y - 3, temp);
}

void animation_clear() {
	memset(disp_damage, 0, sizeof(disp_damage));
	memset(disp_hits, 0, sizeof(disp_hits));
}

void animation_render() {
}

void animation_damage(creaturei* target, int hits) {
	int pind = get_party_index(target);
	if(pind != -1) {
		disp_damage[pind] = hits;
		animation_render();
		disp_damage[pind] = 0;
	} /*else if(target->is(StaticObject))
		animation_render();*/
	else {
		auto e = get_monster_disp(target);
		if(e) {
			short unsigned flags[4];
			memcpy(flags, e->flags, sizeof(e->flags));
			for(auto i = 0; i < 4; i++)
				e->flags[i] |= ImageColor;
			animation_render();
			memcpy(e->flags, flags, sizeof(e->flags));
		}
	}
}

// If hits == -1 the attack is missed
void animation_attack(creaturei* attacker, wearn slot, int hits) {
	auto pind = get_party_index(attacker);
	if(pind != -1) {
		auto sdr = (pind == 0 || pind == 2) ? Left : Right;
		//auto sht = bsdata<itemi>::elements[attacker->get(slot).gettype()].image.shoot;
		//if(sht)
		//	animation_thrown(attacker->getindex(), attacker->getdirection(), sht, sdr, 50, true);
		disp_hits[pind][((slot == RightHand) ? 0 : 1)] = hits;
	} else {
		auto p = get_monster_disp(attacker);
		if(p) {
			//attacker->setframe(p->frame, 4);
			animation_render();
			//attacker->setframe(p->frame, 5);
		}
	}
}

void set_dungeon_tiles(resid type) {
	map_tiles = gres(type);
	render_flipped_wall = 1 * walls_frames;
	render_dungeon = type;
	render_door_type = BRICK;
	render_mirror1 = CellSecrectButton;
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
	}
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
	auto povr = loc->getoverlay(index_start, to(bd, Down));
	auto tile = povr->type;
	if(tile < CellPuller)
		return 0;
	auto frame = get_tile(tile, false);
	if(frame == -1)
		return 0;
	if(tile == CellPuller) {
		if(loc->isactive(povr))
			frame += decor_frames;
	}
	p->frame[1] = frame + n;
	if(use_flip && flip && (render_mirror1 == tile || render_mirror2 == tile))
		p->flags[1] ^= ImageMirrorH;
	return povr;
}

static int get_throw_index(unsigned char type) {
	//switch(type) {
	//case Spear: return 6;
	//case Dart: return 8;
	//case Dagger: return 10;
	//default: return 12;
	//}
	return 12;
}

static void fill_item_sprite(renderi* p, const itemi* pi, int frame = 0) {
	if(pi->is(Large))
		p->rdata = gres(ITEMGL);
	else
		p->rdata = gres(ITEMGS);
	p->frame[frame] = pi->avatar_ground;
}

static void fill_sprite(renderi* p, const itemi* pi, directions drs) {
	//switch(type) {
	//case FireThrown: case LightingThrown: case IceThrown: case MagicThrown:
	//	p->frame[0] = (type - FireThrown) + 2;
	//	p->rdata = gres(THROWN);
	//	break;
	//case Spear:case Dart: case Dagger: case Arrow:
	//	p->frame[0] = get_throw_index(type);
	//	p->rdata = gres(THROWN);
	//	if(drs == Right)
	//		p->flags[0] |= ImageMirrorH;
	//	break;
	//default:
	//	fill_item_sprite(p, type);
	//	break;
	//}
}

static renderi* add_cellar_items(renderi* p, int i, dungeoni::overlayi* povr) {
	if(!povr)
		return p;
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
		//item* result[4];
		//auto item_count = loc->getitems(result, zendof(result), povr);
		//auto d = pos_levels[i] * 2;
		//for(unsigned n = 0; n < item_count; n++) {
		//	p++;
		//	p->clear();
		//	p->x = positions[i].x + n;
		//	p->y = positions[i].y + n / 2;
		//	p->z = pos_levels[i] * distance_per_level;
		//	p->zorder = 2;
		//	p->pos = *povr;
		//	p->percent = item_distances[d][0];
		//	p->alpha = (unsigned char)item_distances[d][1];
		//	fill_item_sprite(p, &result[n]->geti());
		//}
	}
	return p;
}

static renderi* create_wall(renderi* p, int i, pointc index, int frame, celln rec, bool flip) {
	int n;
	// Walls render
	// | |_  5 4 7
	// | |_  6 3 8
	//   |_    2 9
	//   |     1
	static char walls_front[18] = {
		7, 7, 7, 7, 7, 7, 7,
		8, 8, 8, 8, 8,
		9, 9, 9,
		0, 0, 0,
	};
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
	static point wall_position[18] = {{scrx / 2 - 48 * 3, scry / 2},
		{scrx / 2 - 48 * 2, scry / 2},
		{scrx / 2 - 48 * 1, scry / 2},
		{scrx / 2, scry / 2},
		{scrx / 2 + 48 * 1, scry / 2},
		{scrx / 2 + 48 * 2, scry / 2},
		{scrx / 2 + 48 * 3, scry / 2},
		// Level 2
		{scrx / 2 - 80 * 2, scry / 2},
		{scrx / 2 - 80 * 1, scry / 2},
		{scrx / 2, scry / 2},
		{scrx / 2 + 80 * 1, scry / 2},
		{scrx / 2 + 80 * 2, scry / 2},
		// Level 1
		{scrx / 2 - 128, scry / 2},
		{scrx / 2, scry / 2},
		{scrx / 2 + 128, scry / 2},
		// Level 0
		{scrx / 2, scry / 2},
		{scrx / 2, scry / 2},
		{scrx / 2, scry / 2},
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
		return p;
	// Front
	n = walls_front[i];
	if(n) {
		p->clear();
		p->x = wall_position[i].x;
		p->y = scry / 2;
		p->z = pos_levels[i] * distance_per_level;
		p->frame[0] = n + frame;
		p->rdata = map_tiles;
		p->pos = index;
		p->rec = rec;
		auto front_wall = p;
		if(rec == CellDoor && i < 15) {
			p++;
			p->clear();
			p->x = wall_position[i].x;
			p->y = scry / 2;
			p->z = pos_levels[i] * distance_per_level;
			p->zorder = 1;
			p->rdata = map_tiles;
			p->pos = index;
			auto e1 = map_tiles->get(door_offset + pos_levels[i] - 1);
			auto e2 = map_tiles->get(door_offset + 6 + pos_levels[i] - 1);
			auto po = loc->getoverlay(to(indecies[i], to(party.d, Down)), party.d);
			switch(render_door_type) {
			case BLUE:
				if(loc->is(index, CellActive)) {
					if(po) {
						auto w = e1.sx - (e2.sx * 3) / 2;
						p->x -= w;
						p->zorder = 2;
						p->frame[0] = door_offset + 6 + pos_levels[i] - 1;
						p++; memcpy(p, p - 1, sizeof(p[0]));
						p->x += 2 * w;
						p->flags[0] = ImageMirrorH;
						p++;
					}
					return p;
				} else {
					auto p1 = p;
					p->frame[0] = door_offset + pos_levels[i] - 1;
					p++; memcpy(p, p - 1, sizeof(p[0]));
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
				// Drop down door
				if(loc->is(index, CellActive))
					p--;
				else
					p->frame[0] = door_offset + pos_levels[i] - 1;
				if(po)
					front_wall->frame[1] = door_offset + 6 + pos_levels[i] - 1;
				break;
			default:
				// Drop down door
				p->frame[0] = door_offset + pos_levels[i] - 1;
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
			}
			auto povr = add_wall_decor(p, index, Down, decor_front[i], flip, true);
			p = add_cellar_items(p, i, povr);
		}
		p++;
	}
	// Left
	n = walls_left[i];
	enable = true;
	if((rec == CellStairsUp || rec == CellStairsDown) && i != 15)
		enable = false;
	if(n && enable) {
		p->clear();
		p->x = wall_position[i].x;
		if(n == 5 || n == 6)
			p->x += wall_width[i] * 2;
		else
			p->x += wall_width[i];
		p->y = scry / 2;
		p->z = pos_levels[i] * distance_per_level + 2;
		p->frame[0] = n + frame;
		p->rdata = map_tiles;
		p->pos = index;
		p->rec = rec;
		add_wall_decor(p, index, Right, decor_left[i], flip, false);
		p++;
	}
	// Right
	n = walls_right[i];
	enable = true;
	if((rec == CellStairsUp || rec == CellStairsDown) && i != 17)
		enable = false;
	if(n && enable) {
		p->clear();
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
		p->pos = index;
		p->rec = rec;
		p->flags[1] = ImageMirrorH;
		add_wall_decor(p, index, Left, decor_right[i], flip, false);
		p++;
	}
	return p;
}

static renderi* create_floor(renderi* p, int i, pointc index, celln rec, bool flip) {
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
		p->clear();
		p->x = floor_pos[i];
		p->y = scry / 2;
		p->z = pos_levels[i] * distance_per_level + 1;
		if(flip)
			p->flags[0] = ImageMirrorH;
		if(rec == CellButton && loc->is(index, CellActive))
			frame = get_tile_alternate(rec);
		p->frame[0] = floor_frame[i] + frame;
		if(bsdata<celli>::elements[rec].resource)
			p->rdata = gres(bsdata<celli>::elements[rec].resource);
		else
			p->rdata = map_tiles;
		p->pos = index;
		p->rec = rec;
		p++;
	}
	return p;
}

static int get_x_from_line(int y, int x1, int y1, int x2, int y2) {
	return ((y - y1) * (x2 - x1)) / (y2 - y1) + x1;
}

static renderi* create_thrown(renderi* p, int i, int ps, const itemi* rec, directions dr) {
	static int height_sizes[8] = {120, 96, 71, 64, 48, 40, 30, 24};
	p->clear();
	int m = pos_levels[i];
	int d = pos_levels[i] * 2 + (1 - ps / 2);
	int h = height_sizes[d] / 6 - height_sizes[d];
	switch(dr) {
	case Left:
		p->y = 24 + d * 2;
		//p->y = 30 + d * 2;
		p->x = get_x_from_line(p->y, (176 - 72) / 2 + 14, 24, (176 - 32) / 2 + 6, 40);
		break;
	case Right:
		p->y = 24 + d * 2;
		//p->y = 30 + d * 2;
		p->x = get_x_from_line(p->y, (176 - 72) / 2 + 72 - 14, 24, (176 - 32) / 2 + 32 - 6, 40);
		break;
	default:
		p->x = 176 / 2;
		p->y = 40 + d;
		break;
	}
	p->z = pos_levels[i] * distance_per_level + (1 - ps / 2);
	p->pos = indecies[i];
	fill_sprite(p, rec, dr);
	p->percent = item_distances[d][0];
	p->alpha = (unsigned char)item_distances[d][1];
	p++;
	p->rdata = 0;
	return p;
}

static renderi* create_items(renderi* p, int i, pointc index, directions dr) {
	//item* result[64];
	//int item_count = loc->getitems(result, zendof(result), index);
	//for(int n = 0; n < item_count; n++) {
	//	auto it = *result[n];
	//	int ps = game.getside(location.getitemside(result[n]), dr);
	//	int d = pos_levels[i] * 2 + (1 - ps / 2);
	//	p->clear();
	//	p->x = item_position[i * 4 + ps].x;
	//	p->y = item_position[i * 4 + ps].y;
	//	p->z = pos_levels[i] * distance_per_level + 1 + (1 - ps / 2);
	//	p->pos = index;
	//	p->percent = item_distances[d][0];
	//	p->alpha = (unsigned char)item_distances[d][1];
	//	fill_item_sprite(p, &it.geti());
	//	p++;
	//}
	return p;
}

static renderi* create_monsters(renderi* p, int i, pointc index, directions dr, bool flip) {
	//creaturei* result[4]; location.getmonsters(result, index, dr);
	//for(int n = 0; n < 4; n++) {
	//	auto pc = result[n];
	//	if(!pc)
	//		continue;
	//	auto size = pc->getsize();
	//	auto dir = get_absolute_direction(dr, pc->getdirection());
	//	int d = pos_levels[i] * 2 - (n / 2);
	//	p->clear();
	//	if(size == Large) {
	//		p->x = item_position[i * 4 + 0].x + (item_position[i * 4 + 1].x - item_position[i * 4 + 0].x) / 2;
	//		p->y = item_position[i * 4 + 0].y + (item_position[i * 4 + 3].y - item_position[i * 4 + 0].y) / 2;
	//		p->z = pos_levels[i] * distance_per_level - 1;
	//		d = pos_levels[i] * 2 - 1;
	//	} else {
	//		p->x = item_position[i * 4 + n].x;
	//		p->y = item_position[i * 4 + n].y;
	//		p->z = pos_levels[i] * distance_per_level + 1 - (n / 2);
	//	}
	//	p->percent = item_distances[d][0];
	//	p->alpha = (unsigned char)item_distances[d][1];
	//	p->rdata = gres(pc->getres());
	//	if(!p->rdata)
	//		continue;
	//	p->pc = pc;
	//	p->pallette = pc->getpallette();
	//	unsigned flags = 0;
	//	// Анимируем активных монстров
	//	if(((p->x + draw::frametick) / 16) % 2) {
	//		p->x++;
	//		p->y++;
	//	}
	//	switch(dir) {
	//	case Left:
	//		pc->setframe(p->frame, flip ? 2 : 1);
	//		break;
	//	case Right:
	//		pc->setframe(p->frame, flip ? 2 : 1);
	//		flags |= ImageMirrorH;
	//		break;
	//	case Up:
	//		pc->setframe(p->frame, 3);
	//		if(flip)
	//			flags ^= ImageMirrorH;
	//		break;
	//	case Down:
	//		pc->setframe(p->frame, 0);
	//		if(flip)
	//			flags ^= ImageMirrorH;
	//		break;
	//	}
	//	for(int i = 0; i < 4; i++)
	//		p->flags[i] = flags;
	//	p++;
	//}
	return p;
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
	auto p = disp_data;
	// walls
	for(int i = 0; i < 18; i++) {
		int x1 = x + offsets[i * 2 + 0];
		int y1 = y + offsets[i * 2 + 1];
		bool mr = ((x1 + y1 + party.d) & 1) != 0;
		if(x1 < 0 || y1 < 0 || x1 >= mpx || y1 >= mpy) {
			p = create_wall(p, i, {-1, -1}, get_tile(CellWall, mr), CellWall, !mr);
			continue;
		}
		pointc index{(char)x1, (char)y1};
		auto tile = loc->get(index);
		auto tilt = get_wall_type(tile);
		indecies[i] = index;
		if(tilt != CellWall && tilt != CellStairsUp && tilt != CellStairsDown) {
			if(tilt != CellDoor) {
				if(locup && locup->get(index) == CellPit)
					p = create_floor(p, i, index, CellPitUp, mr);
			}
			p = create_items(p, i, index, dr);
			p = create_monsters(p, i, index, dr, mr);
		}
		auto& et = bsdata<celli>::elements[tile];
		//if(et.flags.is(LookWall))
		//	p = create_wall(p, i, index, get_tile(tile, mr), tile, mr);
		//else if(et.flags.is(LookObject))
		//	p = create_floor(p, i, index, tile, mr);
	}
	p->rdata = 0;
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
	const sprite::frame& f = res->get(id);
	int sx = f.sx;
	int sy = f.sy;
	int ssx = f.sx * percent / 1000;
	int ssy = f.sy * percent / 1000;
	int sox = f.ox * percent / 1000;
	int soy = f.oy * percent / 1000;
	unsigned flags_addon = (flags & ImagePallette);
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

void add_color_data(pma* result, const unsigned char* bitdata) {
	palspr* epr = static_cast<palspr*>(result);
	epr->name[0] = 'C';
	epr->name[1] = 'O';
	epr->name[2] = 'L';
	epr->name[3] = 0;
	epr->size = sizeof(epr);
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
	setclip({0, 0, scrx, scry});
	renderi** pz = zorder;
	for(renderi* p = disp_data; p->rdata; p++)
		*pz++ = p;
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
	prepare_draw(party, party.d);
}

renderi* get_last_disp() {
	for(auto& e : disp_data) {
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

//int draw::animation::thrownstep(indext index, direction_s dr, item_s itype, direction_s sdr, int wait) {
//	index = to(index, dr);
//	if(index == Blocked)
//		return index;
//	int i = get_index_pos(index);
//	if(i == -1)
//		return index;
//	auto p = get_last_disp();
//	dr = to(dr, Down);
//	int inc, side;
//	switch(dr) {
//	case Up:
//		side = 2;
//		inc = -2;
//		break;
//	default:
//		side = 0;
//		inc = 2;
//		break;
//	}
//	create_thrown(p, i, side, itype, sdr);
//	draw::animation::render(wait);
//	int x = gx(index);
//	int y = gy(index);
//	if(x < 0 || x >= mpx || y < 0 || y >= mpy)
//		return 0;
//	if(!location.isblocked(index)) {
//		create_thrown(p, i, side + inc, itype, sdr);
//		draw::animation::render(wait);
//	}
//	p->rdata = 0;
//	return index;
//}

//int draw::animation::thrown(indext index, direction_s dr, item_s type, direction_s sdr, int wait, bool block_monsters) {
//	for(int i = 0; i < 3; i++) {
//		int i2 = thrownstep(index, dr, type, sdr, wait);
//		if(i2 == Blocked || location.isblocked(i2))
//			break;
//		index = i2;
//		if(block_monsters) {
//			creature* source[4]; location.getmonsters(source, i2, dr);
//			if(source[0] || source[1] || source[2] || source[3])
//				return i2;
//		}
//	}
//	return index;
//}