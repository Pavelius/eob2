#include "answers.h"
#include "avatar.h"
#include "boost.h"
#include "cell.h"
#include "creature.h"
#include "class.h"
#include "console.h"
#include "direction.h"
#include "draw.h"
#include "dungeon.h"
#include "hotkey.h"
#include "gender.h"
#include "io_stream.h"
#include "location.h"
#include "math.h"
#include "midi.h"
#include "picture.h"
#include "quest.h"
#include "race.h"
#include "resid.h"
#include "timer.h"
#include "party.h"
#include "view.h"
#include "view_focus.h"

using namespace draw;

namespace colors {
static color button(108, 108, 136); // Any button background color
static color light(148, 148, 172); // Light button border color
static color dark(52, 52, 80); // Dark button border color
static color hilite = button.mix(dark, 160); // Hilite button background color
static color focus(250, 100, 100); // Focus text button color
static color form(164, 164, 186); // Character sheet form color
static color info(64, 64, 64); // Character sheet text color
static color down(81, 85, 166); // Green bar background color
static color title(64, 255, 255); // Spells header color
}

static fnevent character_view_proc;
static void* current_select;
static point cancel_position;
static bool hilite_player;
static int disp_damage[6];
static int disp_weapon[6][2];
static unsigned animate_counter;
static int last_spell_level;
static int answer_origin, answer_per_page, answer_index;
bool need_update_animation;
unsigned long current_cpu_time;

static void paint_inventory();

struct pushscene : pushfocus {
	const sprite* font;
	pushscene() : pushfocus(), font(draw::font) {}
	~pushscene() { draw::font = font; }
};

const int animation_step = 300;

template<typename T> const char* gtn(int v) {
	return bsdata<T>::elements[v].getname();
}

static unsigned get_frame_tick() {
	return getcputime() / 10;
}

static int get_alpha(int base, unsigned range, int tick) {
	auto seed = tick % range;
	auto half = range / 2;
	if(seed < half)
		return base * seed / half;
	else
		return base * (range - seed) / half;
}

static int get_alpha(int base, unsigned range) {
	return get_alpha(base, range, get_frame_tick());
}

static const char* namesh(const char* id) {
	auto p = getnme(ids(id, "Short"));
	if(!p)
		p = "XXX";
	return p;
}

static void press_key() {
	hot.key = hot.param;
	hot.pressed = false;
}

void set_small_font() {
	font = gres(FONT6);
}

static void set_big_font() {
	font = gres(FONT8);
}

//static int get_party_disp(creaturei* target, wearn id) {
//	if(!target)
//		return 0;
//	int pind = get_party_index(target);
//	if(pind == -1)
//		return 0;
//	if(id == RightHand)
//		return disp_weapon[pind][0];
//	else if(id == LeftHand)
//		return disp_weapon[pind][1];
//	return 0;
//}

void fix_damage(const creaturei* target, int value) {
	auto i = get_party_index(target);
	if(i == -1)
		fix_monster_damage(target);
	else {
		if(disp_damage[i])
			fix_animate(); // Try add another animation over existing. So we update right now.
		disp_damage[i] = value;
		need_update_animation = true;
	}
}

// If hits == -1 the attack is missed
void fix_attack(const creaturei* attacker, wearn slot, int hits) {
	auto pind = get_party_index(attacker);
	if(pind == -1)
		return;
	// If thrown animation fix attack
	auto avatar_thrown = attacker->wears[slot].geti().avatar_thrown;
	if(avatar_thrown)
		thrown_item(party, Up, avatar_thrown, pind % 2, enemy_distance);
	if(disp_weapon[pind][((slot == RightHand) ? 0 : 1)] != -1)
		fix_animate();
	disp_weapon[pind][((slot == RightHand) ? 0 : 1)] = hits;
	need_update_animation = true;
}

static void copy_image(point origin, point dest, int w, int h) {
	auto scan_size = sizeof(color) * w;
	for(auto i = 0; i < h; i++)
		memcpy(canvas->ptr(dest.x, dest.y + i), canvas->ptr(origin.x, origin.y + i), scan_size);
}

static void paint_background(resid v, int frame) {
	current_cpu_time = getcputime();
	draw::image(gres(v), frame, 0);
}

static void paint_picture() {
	image(0, 0, gres(BORDER), 0, 0);
	if(!picture)
		return;
	rectpush push;
	caret.x = 8; caret.y = 8; width = 160; height = 96;
	image(8, 8, gres(picture.res), picture.frame, 0);
}

static void button_back(bool focused) {
	rectpush push;
	auto push_fore = fore;
	fore = focused ? colors::hilite : colors::button;
	width++;
	height++;
	rectf();
	fore = push_fore;
}

static void border_up() {
	auto push_fore = fore;
	auto push_caret = caret;
	fore = colors::light;
	line(caret.x + width, caret.y);
	line(caret.x, caret.y + height);
	fore = colors::dark;
	line(caret.x - width, caret.y);
	line(caret.x, caret.y - height + 1);
	caret = push_caret;
	fore = push_fore;
}

static void border_down() {
	auto push_fore = fore;
	auto push_caret = caret;
	fore = colors::dark;
	line(caret.x + width, caret.y);
	line(caret.x, caret.y + height);
	fore = colors::light;
	line(caret.x - width, caret.y);
	line(caret.x, caret.y - height + 1);
	caret = push_caret;
	fore = push_fore;
}

static void button_frame(int count, bool focused, bool pressed) {
	rectpush push;
	for(int i = 0; i < count; i++) {
		if(pressed)
			border_down();
		else
			border_up();
		setoffset(1, 1);
	}
	button_back(focused);
}

static bool button_input(const void* button_data, unsigned key, unsigned key_hot = 0xFFFF0000) {
	if(!button_data)
		return false;
	auto ishilited = ishilite();
	auto isfocused = (current_focus == button_data);
	if(hot.key == MouseLeft && hot.pressed && ishilited && current_focus != button_data)
		execute(cbsetptr, (long)button_data, 0, &current_focus);
	else if((isfocused && (hot.key == KeyEnter || hot.key == key_hot)) || (key && hot.key == key) || (ishilited && hot.pressed))
		pressed_focus = (void*)button_data;
	else if((hot.key == InputKeyUp && pressed_focus == button_data) || (ishilited && hot.key == MouseLeft && !hot.pressed)) {
		pressed_focus = 0;
		return true;
	}
	return false;
}

static void button_press_effect() {
	auto size = sizeof(color) * (width - 1);
	for(auto y = caret.y + height - 2; y >= caret.y; y--) {
		memmove(canvas->ptr(caret.x + 1, y + 1), canvas->ptr(caret.x, y), size);
		*((color*)canvas->ptr(caret.x, y)) = color();
		*((color*)canvas->ptr(caret.x, y + 1)) = color();
	}
	memset(canvas->ptr(caret.x, caret.y), 0, width * sizeof(color));
}

static bool button(rect rc) {
	if(disable_input)
		return false;
	rectpush push;
	caret.x = rc.x1;
	caret.y = rc.y1;
	width = rc.width();
	height = rc.height();
	auto button_data = (void*)(*((int*)&caret));
	auto ishilited = ishilite();
	auto isfocused = (current_focus == button_data);
	auto run = false;
	if(ishilited && hot.pressed)
		pressed_focus = (void*)button_data;
	else if(ishilited && hot.key == MouseLeft && !hot.pressed) {
		pressed_focus = 0;
		run = true;
	}
	if(pressed_focus == button_data)
		button_press_effect();
	return run;
}

static bool button(resid id, int normal, int pressed, int overlay, unsigned key) {
	auto ps = gres(id);
	rectpush push; width = ps->get(normal).sx; height = ps->get(normal).sy;
	auto button_data = (void*)(*((int*)&caret));
	auto run = button_input(button_data, key);
	auto frame = (pressed_focus == button_data) ? pressed : normal;
	image(ps, frame, 0);
	if(overlay != -1) {
		auto& f1 = ps->get(frame);
		auto& f2 = ps->get(overlay);
		if(f1.sx > f2.sx)
			caret.x += (f1.sx - f2.sx + 1) / 2;
		if(f1.sy > f2.sy)
			caret.y += (f1.sy - f2.sy + 1) / 2;
		image(ps, overlay, 0);
	}
	return run;
}

static void button(resid id, int normal, int pressed, int overlay, unsigned key, fnevent proc, long param = 0, void* param_object = 0) {
	if(button(id, normal, pressed, overlay, key))
		execute(proc, param, 0, param_object);
}

static void button(rect rc, fnevent proc, long param = 0, void* param_object = 0) {
	if(button(rc))
		execute(proc, param, 0, param_object);
}

static void correct_answers(int maximum) {
	if(answer_index >= maximum)
		answer_index = maximum - 1;
	if(answer_index < 0)
		answer_index = 0;
	if(answer_index < answer_origin)
		answer_origin = answer_index;
	if(answer_index >= answer_origin + answer_per_page)
		answer_origin = answer_index - answer_per_page + 1;
	if(answer_origin + answer_per_page >= maximum)
		answer_origin = maximum - answer_per_page;
	if(answer_origin < 0)
		answer_origin = 0;
	if(answer_index < answer_origin)
		answer_index = answer_origin;
	if(answer_index > answer_origin + answer_per_page)
		answer_index = answer_origin + answer_per_page - 1;
}

static void textc(color tc, const char* format, ...) {
	char temp[32]; stringbuilder sb(temp);
	XVA_FORMAT(format);
	sb.addv(format, format_param);
	auto push_fore = fore;
	auto push_caret = caret;
	fore = tc;
	caret.x -= textw(temp) / 2;
	caret.y -= texth() / 2;
	text(temp);
	caret = push_caret;
	fore = push_fore;
}

static void paint_player_hit(int hits, unsigned counter) {
	image(caret.x, caret.y - 1, gres(THROWN), 1, (counter % 2) ? ImageMirrorH : 0);
	if(hits == -2)
		textc(colors::white, "hack", hits);
	else if(hits == -1)
		textc(colors::white, "miss", hits);
	else
		textc(colors::white, "%1i", hits);
}

static void paint_player_hit(const creaturei* player, wearn id) {
	if(!player)
		return;
	auto pind = get_party_index(player);
	if(pind == -1)
		return;
	auto value = disp_weapon[pind][id == RightHand ? 0 : 1];
	if(!value)
		return;
	auto push_caret = caret;
	caret.x += width / 2;
	caret.y += height / 2;
	paint_player_hit(value, animate_counter + pind);
	caret = push_caret;
}

static void paint_player_damage(int hits, unsigned counter) {
	image(caret.x, caret.y - 1, gres(THROWN), 0, (counter % 2) ? ImageMirrorH : 0);
	textc(colors::white, "%1i", hits);
}

static bool paint_button(const char* title, const void* button_data, unsigned key, unsigned flags = TextBold, bool force_focus = false) {
	rectpush push;
	auto push_fore = fore;
	if(!button_data)
		button_data = (void*)title;
	focusing(button_data);
	auto run = button_input(button_data, key);
	auto pressed = (pressed_focus == button_data);
	button_frame(1, false, pressed);
	if((current_focus == button_data) || force_focus)
		fore = colors::focus;
	caret.y += 2;
	caret.x += 4;
	width -= 4 * 2;
	if(pressed) {
		caret.x++;
		caret.y++;
	}
	text(title, -1, flags);
	fore = push_fore;
	return run;
}

static void paint_answers(fnanswer paintcell, fnevent pushbutton, int height_grid) {
	if(!paintcell)
		return;
	int index = answer_origin;
	int index_stop = an.elements.count;
	if(answer_per_page != -1) {
		index_stop = index + answer_per_page;
		if(index_stop > (int)an.elements.count)
			index_stop = (int)an.elements.count;
	}
	while(true) {
		if(index >= index_stop)
			break;
		auto& e = an.elements[index];
		paintcell(index, &e, e.text, e.key, pushbutton);
		caret.y += height_grid;
		index++;
	}
}

void text_label(int index, const void* data, const char* format, unsigned key, fnevent proc) {
	auto push_fore = fore;
	focusing(data);
	if(button_input(data, key))
		execute(proc, (long)data);
	fore = colors::white;
	if(current_focus == data)
		fore = colors::focus;
	if(pressed_focus == data)
		fore = fore.darken();
	texta(format, AlignCenter | TextBold);
	fore = push_fore;
}

void text_label_menu(int index, const void* button_data, const char* format, unsigned key, fnevent proc) {
	auto push_fore = fore;
	if(!button_data)
		button_data = (void*)format;
	focusing(button_data);
	if(button_input(button_data, key, 'E'))
		execute(proc, (long)button_data);
	if(current_focus == button_data) {
		rectpush push;
		caret.x -= 2; caret.y -= 1;
		fore = colors::dark;
		rectf();
		caret = push.caret;
		caret.x += width - textw('1') - 2;
		fore = colors::white;
		if(index == answer_origin && answer_origin != 0)
			paint_arrow(caret, Up, 6);
		else if(answer_per_page != -1 && index == (answer_origin + answer_per_page - 1) && (answer_origin + answer_per_page < (int)an.getcount()))
			paint_arrow(caret, Down, 6);
	}
	if(index >= 1000)
		fore = colors::white.mix(colors::dark, 196);
	else
		fore = colors::white;
	if(pressed_focus == button_data)
		fore = fore.darken();
	texta(format, 0);
	fore = push_fore;
}

void text_label_left(int index, const void* data, const char* format, unsigned key, fnevent proc) {
	auto push_fore = fore;
	focusing(data);
	if(button_input(data, key))
		execute(proc, (long)data);
	fore = colors::white;
	if(current_focus == data)
		fore = colors::focus;
	if(pressed_focus == data)
		fore = fore.darken();
	texta(format, TextBold);
	fore = push_fore;
}

static void label_control(const char* format, const void* data, unsigned flags) {
	auto push_fore = fore;
	fore = colors::white;
	if(current_focus == data)
		fore = colors::focus;
	if(pressed_focus == data)
		fore = fore.darken();
	texta(format, flags);
	fore = push_fore;
}

void button_label(int index, const void* data, const char* format, unsigned key, fnevent proc) {
	if(paint_button(format, data, key))
		execute(proc, (long)data);
}

static void update_buttonparam() {
	updatewindow();
	buttonparam();
}

void set_player_by_focus() {
	auto i = bsdata<creaturei>::source.indexof(current_focus);
	if(i != -1)
		player = bsdata<creaturei>::elements + i;
}

void set_focus_by_player() {
	if(player) {
		if(current_focus == player->wears + RightHand)
			return;
		if(current_focus == player->wears + LeftHand)
			return;
		current_focus = player->wears + RightHand;
	}
}

static void clear_page() {
	character_view_proc = 0;
	set_focus_by_player();
}

static void switch_page(fnevent proc) {
	if(character_view_proc == proc)
		clear_page();
	else {
		set_player_by_focus();
		character_view_proc = proc;
	}
}

static int get_compass_index(directions d) {
	switch(d) {
	case Right: return 1; // East
	case Down: return 2; // South
	case Left: return 3; // West
	default: return 0; // North
	}
}

static void paint_compass(directions d) {
	auto i = get_compass_index(d);
	image(114, 132, gres(COMPASS), i, 0);
	image(79, 158, gres(COMPASS), 4 + i, 0);
	image(150, 158, gres(COMPASS), 8 + i, 0);
	button({5, 128, 24, 144}, press_key, KeyHome);
	button({24, 128, 44, 144}, press_key, KeyUp);
	button({44, 128, 64, 144}, press_key, KeyPageUp);
	button({5, 145, 24, 161}, press_key, KeyLeft);
	button({24, 145, 44, 161}, press_key, KeyDown);
	button({44, 145, 64, 161}, press_key, KeyRight);
}

static void paint_menu(point position, int object_width, int object_height) {
	rectpush push;
	caret = position;
	width = object_width;
	height = object_height;
	button_frame(2, false, false);
}

static void paint_small_menu(point position, int object_width, int object_height) {
	rectpush push;
	set_small_font();
	caret = position;
	width = object_width;
	height = object_height;
	button_frame(1, false, false);
}

static void paint_blend(color new_color, unsigned new_alpha) {
	auto push_alpha = alpha;
	auto push_fore = fore;
	fore = new_color;
	alpha = new_alpha;
	rectf();
	fore = push_fore;
	alpha = push_alpha;
}

static void blend_avatar(slice<color> source, unsigned char intensity = 64, unsigned range = 256) {
	if(!source)
		return;
	auto index = (get_frame_tick() / range) % source.size();
	paint_blend(source[index], get_alpha(intensity, range));
}

static void paint_avatar_stats() {
	adat<color, 8> avatar_colors;
	avatar_colors.clear();
	if(player->is(PoisonLevel) || player->is(DiseaseLevel))
		avatar_colors.add(colors::green);
	// Only bad condition must be colored
	//if(player->is(ImmuneFire) || player->is(ResistFire))
	//	avatar_colors.add(colors::red);
	//if(player->is(ImmuneCold) || player->is(ResistCold))
	//	avatar_colors.add(colors::blue);
	blend_avatar(avatar_colors, 32);
}

static void paint_avatar_border() {
	adat<color, 8> avatar_colors;
	referencei target = player;
	for(auto& e : bsdata<boosti>()) {
		if(e.target != target)
			continue;
		if(e.type == BoostSpell) {
			auto ps = bsdata<spelli>::elements + e.param;
			if(!ps->lighting)
				continue;
			avatar_colors.add(ps->lighting);
		}
	}
	if(avatar_colors) {
		auto push_fore = fore;
		auto index = (get_frame_tick() / 256) % avatar_colors.getcount();
		auto alpha = get_alpha(1024, 256);
		if(alpha >= 256)
			fore = avatar_colors[index];
		else
			fore = avatar_colors[index].mix(colors::black, alpha);
		rectb();
		fore = push_fore;
	}
}

static void paint_shadow() {
	auto push = alpha; alpha = 64;
	rectf();
	alpha = push;
}

static bool mouse_button() {
	if(!ishilite())
		return false;
	if(hot.pressed)
		paint_shadow();
	if(hot.key == MouseLeft && !hot.pressed)
		return true;
	return false;
}

static bool mouse_button(void* button_data, unsigned key) {
	if(!button_data)
		return false;
	if(mouse_button())
		return true;
	if(key && hot.key == key)
		pressed_focus = (void*)button_data;
	else if(hot.key == InputKeyUp && pressed_focus == button_data) {
		pressed_focus = 0;
		return true;
	}
	return false;
}

static void apply_switch_page() {
	player = (creaturei*)hot.param;
	set_focus_by_player();
	if(character_view_proc)
		clear_page();
	else
		switch_page(paint_inventory);
}

static void paint_avatar() {
	if(player->avatar == 0xFF)
		return;
	rectpush push; width = 31; height = 32;
	auto push_alpha = alpha;
	if(player->is(Invisibled))
		alpha = 128;
	if(player->isdead())
		image(gres(PORTM), 0, 0);
	else
		image(gres(PORTM), player->avatar, 0);
	alpha = push_alpha;
	paint_avatar_stats();
	auto pind = get_party_index(player);
	if(pind != -1) {
		auto v = disp_damage[pind];
		if(v) {
			auto push_caret = caret;
			caret.x += 16; caret.y += 16;
			paint_player_damage(v, (animate_counter + pind) % 2);
		}
	}
	button({push.caret.x, push.caret.y, push.caret.x + 31, push.caret.y + 32}, apply_switch_page, (long)player);
}

static void greenbar(int vc, int vm) {
	if(!vm)
		return;
	if(vc < 0)
		vc = 0;
	auto push_fore = fore;
	rectpush push;
	color c1 = colors::green.darken().mix(colors::red, vc * 255 / vm);
	border_down();
	caret.x++; caret.y++;
	width--; height--;
	fore = colors::down;
	rectf();
	if(vc) {
		width = vc * width / vm;
		fore = c1;
		rectf();
	}
	fore = push_fore;
}

static void paint_focus_rect() {
	auto push_fore = fore;
	fore = colors::white;
	rectb();
	fore = push_fore;
}

static void paint_hilite_rect() {
	static point cash_caret;
	static unsigned long cash_time;
	if(cash_caret != caret || !cash_time) {
		cash_caret = caret;
		cash_time = get_frame_tick();
	}
	auto push_fore = fore;
	fore = colors::black.mix(colors::white, get_alpha(255, 160, get_frame_tick() - cash_time));
	rectb();
	fore = push_fore;
}

static void paint_select_rect() {
	auto push_fore = fore;
	fore = colors::focus;
	if(current_select == current_focus)
		fore = fore.mix(colors::white);
	rectb();
	fore = push_fore;
}

static void paint_disabled() {
	auto push_alpha = alpha;
	auto push_fore = fore;
	alpha = 128;
	fore = colors::black;
	rectf();
	fore = push_fore;
	alpha = push_alpha;
}

static void focus_and_pick_up_item() {
	current_focus = (void*)hot.param;
	pick_up_item();
}

static void paint_item(item& it, wearn id, int emphty_avatar = -1, int pallette_feat = -1, bool show_disabled = false) {
	rectpush push;
	if(pallette_feat == -1) {
		if(player->is(SeeCursed) && it.iscursed())
			pallette_feat = SeeCursed;
		else if(player->is(SeeMagical) && it.ismagical())
			pallette_feat = SeeMagical;
	}
	if(!disable_input) {
		focusing(&it);
		if(current_select == &it)
			paint_select_rect();
		else if(current_focus == &it)
			paint_focus_rect();
		if(ishilite()) {
			if(hot.key == MouseLeft && !hot.pressed) {
				if(current_select)
					execute(focus_and_pick_up_item, (long)&it);
				else if(current_focus == &it)
					execute(pick_up_item);
				else
					execute(cbsetptr, (long)&it, 0, &current_focus);
			}
		}
	}
	auto avatar = it.geti().avatar;
	if(!it)
		avatar = emphty_avatar;
	if(avatar != -1) {
		auto rs = gres(ITEMS);
		if(pallette_feat) {
			color pallette[256];
			auto& e = rs->get(avatar);
			auto p = (color*)rs->ptr(e.pallette);
			memcpy(pallette, p, sizeof(pallette));
			switch(pallette_feat) {
			case SeeMagical:
				pallette[12] = colors::blue.mix(colors::white, get_alpha(140, 160));
				break;
			case SeeCursed:
				pallette[12] = colors::red.mix(colors::white, get_alpha(140, 160));
				break;
			}
			draw::palt = pallette;
			image(caret.x + width / 2, caret.y + height / 2, rs, avatar, ImagePallette);
		} else
			image(caret.x + width / 2, caret.y + height / 2, gres(ITEMS), avatar, 0);
	}
	auto count = it.getcount();
	if(count > 1) {
		auto ps = str("%1i", count);
		caret.y = push.caret.y + push.height - 6 - 2;
		caret.x = push.caret.x + push.width - textw(ps) - 3;
		auto push_fore = fore;
		fore = colors::white;
		text(ps, -1, TextBold);
		caret = push.caret;
		fore = push_fore;
	}
	if(show_disabled)
		paint_disabled();
}

static void paint_ring(item& it, wearn id) {
	rectpush push;
	width = height = 10;
	paint_item(it, id);
}

static void prev_character() {
	auto index = get_party_index(player);
	if(index <= 0)
		return;
	if(characters[index - 1]) {
		player = characters[index - 1];
		set_focus_by_player();
	}
}

static void next_character() {
	auto index = get_party_index(player);
	if(index < 0 || index >= 5)
		return;
	if(characters[index + 1]) {
		player = characters[index + 1];
		set_focus_by_player();
	}
}

static void next_sheet_page();

static void paint_sheet_head() {
	const point origin = {178, 0};
	caret = origin;
	set_small_font();
	image(gres(INVENT), 0, 0);
	caret.x = origin.x + 4;
	caret.y = origin.y + 3;
	paint_avatar();
	caret.x = origin.x + 38;
	caret.y = origin.y + 6;
	width = 98; height = texth();
	fore = colors::black;
	texta(player->getname(), AlignCenter);
	caret.x = origin.x + 70;
	caret.y = origin.y + 16;
	width = 65;
	height = 5;
	greenbar(player->hp, player->hpm);
	caret.y = origin.y + 25;
	greenbar(player->food, player->getfood());
	caret.x = origin.x + 2;
	caret.y = origin.y + 36;
	width = 140; height = 131;
	fore = colors::info;
	button({274, 36, 293, 50}, prev_character);
	button({297, 36, 316, 50}, next_character);
	button({302, 149, 319, 166}, next_sheet_page);
}

static void paint_blank() {
	auto push_caret = caret;
	auto push_fore = fore;
	auto push_width = width;
	auto push_height = height;
	fore = colors::form;
	height -= 19; rectf();
	caret.y = caret.y + height; height = 18; width -= 19; rectf();
	caret = push_caret; width = push_width; height = push_height;
	fore = color(208, 208, 216);
	caret.x = 274; caret.y = 35;
	line(319, caret.y);
	line(319, 147);
	fore = push_fore;
	caret = push_caret;
	caret.x += 1; caret.y += 2;
}

static void textn(const char* format) {
	text(format);
	caret.y += texth() + 1;
}

static void textr(const char* format) {
	auto push_caret = caret;
	caret.x = caret.x + width - textw(format);
	text(format);
	caret = push_caret;
}

static void textn(const char* format, int value, const char* value_format = 0, int value2 = 0) {
	char temp[260]; stringbuilder sb(temp);
	if(!value_format)
		value_format = "%1i";
	sb.add(value_format, value, value2);
	textr(temp);
	text(format);
	caret.y += texth() + 1;
}

static void addv(stringbuilder& sb, const dice& value) {
	sb.add("%1i-%2i", value.minimum(), value.maximum());
}

static int get_weapon_attack_bonus(wearn w) {
	int bonus = 0;
	player->getdamage(bonus, w, false);
	return bonus;
}

static void add_value(stringbuilder& sb, abilityn id) {
	int bonus;
	switch(id) {
	case AttackMelee:
		sb.add("%1i", 20 - get_weapon_attack_bonus(RightHand));
		break;
	case DamageMelee:
		bonus = 0;
		addv(sb, player->getdamage(bonus, RightHand, false));
		break;
	case AC:
		sb.add("%1i", 10 - player->get(id));
		break;
	case Hits:
		sb.add("%1i", player->hpm);
		break;
	case ReactionBonus:
		sb.add("%+1i", player->get(id));
		break;
	case Strenght:
		if(player->get(id) == 18) {
			auto exeptional = player->get(ExeptionalStrenght);
			if(exeptional == 100)
				sb.add("18/00");
			else
				sb.add(str("18/%1.2i", exeptional));
		} else
			sb.add("%1i", player->get(id));
		break;
	default:
		sb.add("%1i", player->get(id));
		break;
	}
}

static void textn(abilityn id) {
	char temp[260]; stringbuilder sb(temp);
	add_value(sb, id); textr(temp);
	textn(namesh(bsdata<abilityi>::elements[id].id));
}

void header_yellow(const char* format) {
	auto push_fore = fore;
	fore = colors::yellow;
	text(format, -1);
	caret.y += texth() + 1;
	fore = push_fore;
}
static void header_yellow_spells_by_level(int level, int max_level) {
	auto push_caret = caret;
	auto push_with = width; width = 12;
	auto push_heigh = height; height = 7;
	auto push_fore = fore; fore = colors::yellow;
	caret.x -= 2; caret.y -= 2;
	for(auto i = 1; i <= max_level; i++) {
		button_frame(1, false, (level == i));
		caret.y += 2;
		texta(str("%1i", i), AlignCenterCenter);
		caret.y -= 2;
		auto hot_key = '1' + i - 1;
		caret.x += width;
	}
	height = push_heigh;
	width = push_with;
	caret = push_caret;
	fore = push_fore;
	caret.y += texth() + 1;
}

static void header_yellow_spells(const char* format) {
	header_yellow_spells_by_level(last_spell_level, 9);
}

static void header(const char* format) {
	auto push_fore = fore;
	auto push_stroke = fore_stroke;
	fore_stroke = fore;
	fore = colors::yellow;
	text(format, -1, TextBold);
	caret.y += texth() * 2;
	fore = push_fore;
	fore_stroke = push_stroke;
}

static void headern(const char* format) {
	auto push_fore = fore;
	auto push_stroke = fore_stroke;
	fore_stroke = fore;
	fore = colors::yellow;
	text(format, -1, TextBold);
	fore = push_fore;
	fore_stroke = push_stroke;
}

static void paint_ability() {
	rectpush push;
	for(auto i = Strenght; i <= Charisma; i = (abilityn)(i + 1)) {
		auto value = player->get(i);
		auto name = bsdata<abilityi>::elements[i].getname();
		if(i == Strenght && value == 18)
			textn(name, player->get(ExeptionalStrenght), "18/%01i");
		else
			textn(name, value);
	}
}

static void paint_stats() {
	rectpush push;
	textn(AttackMelee);
	textn(AC);
	textn(Hits);
	textn(DamageMelee);
	textn(Speed);
	textn(ReactionBonus);
}

static void paint_level_experience() {
	rectpush push;
	headern(getnm("Class"));
	caret.x = push.caret.x + 6 * 7;
	headern(getnm("LevelShort"));
	caret.x = push.caret.x + 6 * 11;
	headern(getnm("ExperienceShort"));
	caret.y += texth() + 2;
	auto push_caret = caret;
	auto& pc = player->getclass();
	auto exp = player->experience / pc.count;
	for(int i = 0; i < pc.count; i++) {
		auto m = pc.classes[i];
		caret.x = push.caret.x;
		text(bsdata<classi>::elements[pc.classes[i]].getname());
		caret.x = push.caret.x + 6 * 8;
		text(str("%1i", player->levels[i]));
		caret.x = push.caret.x + 6 * 11;
		text(str("%1i", exp));
		caret.y += texth() + 1;
	}
}

static void paint_sheet() {
	rectpush push;
	auto push_font = font;
	auto push_fore = fore;
	paint_sheet_head();
	paint_blank();
	header(getnm("Characterinfo"));
	textn(player->getclass().getname());
	textn(bsdata<alignmenti>::elements[player->alignment].getname());
	textn(str("%1 %2", player->getrace().getname(), bsdata<genderi>::elements[player->gender].getname()));
	caret.y += texth();
	auto push_block = caret;
	width = 88;
	paint_ability();
	caret.x += 6 * 16;
	width = 40;
	paint_stats();
	caret.x = push_block.x;
	caret.y = push_block.y + 68;
	paint_level_experience();
	font = push_font;
	fore = push_fore;
}

static void paint_skills() {
	rectpush push;
	auto push_font = font;
	auto push_fore = fore;
	paint_sheet_head();
	paint_blank();
	header(getnm("CharacterSkills"));
	width = 136;
	for(auto i = (abilityn)SaveVsParalization; i <= DetectSecrets; i = (abilityn)(i + 1)) {
		auto value = player->get(i);
		if(value <= 0)
			continue;
		textn(bsdata<abilityi>::elements[i].getname(), player->get(i), "%1i%%");
	}
	font = push_font;
	fore = push_fore;
}

#ifdef _DEBUG
static void paint_feats() {
	rectpush push;
	auto push_font = font;
	auto push_fore = fore;
	paint_sheet_head();
	paint_blank();
	header(getnm("CharacterFeats"));
	width = 136;
	for(auto i = (featn)NoExeptionalStrenght; i <= ImmuneIllusion; i = (featn)(i + 1)) {
		auto value = player->is(i);
		if(!value)
			continue;
		textn(bsdata<feati>::elements[i].getname());
	}
	font = push_font;
	fore = push_fore;
}
#endif // _DEBUG

static void warning(const char* format, unsigned flags) {
	auto push_fore = fore;
	fore = colors::text.mix(colors::red);
	texta(format, flags);
	fore = push_fore;
}

static void paint_states() {
	auto pn = player->getbadstate();
	if(pn)
		warning(getnm(pn), AlignCenter);
}

static void paint_inventory() {
	const int dx = 18;
	static point points[] = {
		{2 + 0 * dx, 39 + 0 * dx},
		{2 + 1 * dx, 39 + 0 * dx},
		{2 + 0 * dx, 39 + 1 * dx},
		{2 + 1 * dx, 39 + 1 * dx},
		{2 + 0 * dx, 39 + 2 * dx},
		{2 + 1 * dx, 39 + 2 * dx},
		{2 + 0 * dx, 39 + 3 * dx},
		{2 + 1 * dx, 39 + 3 * dx},
		{2 + 0 * dx, 39 + 4 * dx},
		{2 + 1 * dx, 39 + 4 * dx},
		{2 + 0 * dx, 39 + 5 * dx},
		{2 + 1 * dx, 39 + 5 * dx},
		{2 + 0 * dx, 39 + 6 * dx},
		{2 + 1 * dx, 39 + 6 * dx},
		{119, 54}, // Head
		{108, 74}, // Neck
		{45, 75}, // Body
		{60 - 9, 124 - 9}, // RightHand
		{108 - 9, 124 - 9}, // LeftHand
		{54 - 5, 140 - 5}, // RightRing
		{66 - 5, 140 - 5}, // LeftRing
		{55 - 9, 104 - 9}, // Elbow
		{107 - 9, 145 - 9}, // Legs
		{55 - 9, 64 - 9}, // Quiver
		{130 - 9, 102 - 9}, // FirstBell
		{130 - 9, 120 - 9}, // SecondBelt
		{130 - 9, 138 - 9}, // LastBell
	};
	rectpush push;
	auto push_font = font;
	auto push_fore = fore;
	paint_sheet_head();
	wearn id = (wearn)0;
	width = dx;
	height = dx;
	for(auto pt : points) {
		caret = pt;
		caret.x += 178;
		if(id == LeftRing || id == RightRing)
			paint_ring(player->wears[id], id);
		else
			paint_item(player->wears[id], id);
		id = (wearn)(id + 1);
	}
	caret.x = 219; caret.y = 159; width = 80; height = texth();
	paint_states();
	font = push_font;
	fore = push_fore;
	button({237, 38, 265, 52}, use_item);
}

static void paint_character() {
	rectpush push;
	auto push_font = font;
	auto push_caret = caret;
	auto push_fore = fore; fore = colors::black;
	width = 65;
	height = 52;
	paint_avatar_border();
	set_small_font();
	caret.y = push.caret.y + 3;
	texta(player->getname(), AlignCenter);
	width = 31;
	height = 16;
	caret.x = push.caret.x + 33;
	caret.y = push.caret.y + 10;
	paint_item(player->wears[RightHand], RightHand, 84);
	paint_player_hit(player, RightHand);
	caret.y = push.caret.y + 26;
	auto disable_offhand = player->wears[RightHand] && player->wears[RightHand].is(TwoHanded);
	paint_item(player->wears[LeftHand], LeftHand, 83, -1, disable_offhand);
	paint_player_hit(player, LeftHand);
	caret.x = push.caret.x + 2;
	caret.y = push.caret.y + 10;
	width = 31;
	paint_avatar();
	caret.y = push.caret.y + 44;
	width = 65;
	texta(str("%1i of %2i", player->gethp(), player->hpm), AlignCenter);
	fore = push_fore;
	font = push_font;
	caret = push_caret;
}

static void paint_character(bool disabled, bool hilite) {
	rectpush push;
	width = 65; height = 52;
	paint_character();
	if(disabled)
		paint_disabled();
	if(hilite)
		paint_hilite_rect();
}

void paint_avatars() {
	static point points[] = {{183, 1}, {255, 1}, {183, 53}, {255, 53}, {183, 105}, {255, 105}};
	rectpush push;
	auto push_player = player;
	if(characters[4])
		copy_image({183, 53}, {183, 105}, 65, 52);
	if(characters[5])
		copy_image({255, 53}, {255, 105}, 65, 52);
	for(auto i = 0; i < 6; i++) {
		caret = points[i];
		player = characters[i];
		if(!player)
			continue;
		paint_character(player->isdisabled() || player->is(Paralized),
			hilite_player && (player == push_player));
	}
	player = push_player;
	button({289, 178, 319, 198}, press_key, KeyEscape);
}

void paint_avatars_no_focus() {
	auto push_input = disable_input; disable_input = true;
	paint_avatars();
	disable_input = push_input;
}

void paint_avatars_no_focus_hilite() {
	auto push_hilite = hilite_player; hilite_player = true;
	auto push_input = disable_input; disable_input = true;
	paint_avatars();
	disable_input = push_input;
	hilite_player = push_hilite;
}

static void paint_party_sheets() {
	if(character_view_proc)
		character_view_proc();
	else
		paint_avatars();
}

static void get_closed_goals(goala goals) {
	if(!last_quest)
		return;
	for(auto& e : last_quest->dungeon) {
		for(auto i = (goaln)0; i <= KillAlmostAllMonsters; i = (goaln)(i + 1)) {
			if(e.is(i))
				goals[i]++;
		}
	}
}

static void paint_quest_goals() {
	goala goals = {}; get_closed_goals(goals);
	rectpush push;
	auto push_font = font;
	auto push_fore = fore;
	paint_sheet_head();
	paint_blank();
	if(last_quest) {
		header(getnm("QuestGoals"));
		width -= 2;
		for(auto i = (goaln)0; i <= KillAlmostAllMonsters; i = (goaln)(i + 1)) {
			if(last_quest->goals[i] <= 0 && goals[i] <= 0)
				continue;
			textn(bsdata<goali>::elements[i].getname(), last_quest->goals[i], "%2i/%1i", goals[i]);
		}
	}
	font = push_font;
	fore = push_fore;
}

static void field(const char* header, int title_width, int total, int value, int maximum) {
	if(value > maximum)
		value = maximum;
	else if(value < 0)
		value = 0;
	auto push_width = width;
	auto push_height = height;
	auto push_caret = caret;
	text(header);
	caret.x += title_width;
	width = total - title_width;
	height = 4;
	greenbar(value, maximum);
	width = push_width;
	height = push_height;
	caret = push_caret;
	caret.y += texth();
}

static void texta(const char* format, unsigned flags, color text_color) {
	auto push_fore = fore; fore = text_color;
	texta(format, flags);
	fore = push_fore;
}

static void party_status_text() {
	char temp[64]; stringbuilder sb(temp);
	sb.add(getnm("PartyStatusFormat"));
	texta(temp, AlignLeft);
}

void paint_party_status() {
	rectpush push;
	auto push_font = font;
	set_small_font();
	paint_menu({0, 122}, 178, 52);
	caret.x = 8; caret.y = 126;
	width = 160; height = texth();
	texta(party.getlocation()->getname(), AlignCenter, colors::yellow);
	caret.y += texth() + 3;
	if(is_dead_line()) {
		auto v = getparty(Minutes);
		auto v1 = getparty(StartDeadLine);
		auto v2 = getparty(StopDeadLine);
		if(v1 < v && v < v2)
			field(getnm("Time"), 64, 160, v2 - v, v2 - v1);
	}
	field(gtn<partystati>(Reputation), 64, 160, getparty(Reputation), 100);
	field(gtn<partystati>(Blessing), 64, 160, getparty(Blessing), 100);
	party_status_text();
	font = push_font;
}

static void paint_console() {
	rectpush push;
	auto push_font = font;
	set_small_font();
	caret = {5, 180};
	width = 280; height = 6 * 3;
	texta(console_text, AlignLeft);
	font = push_font;
}

static void update_focus_player() {
	player = item_owner(current_focus);
}

void paint_city_menu() {
	paint_background(PLAYFLD, 0);
	paint_compass(party.d);
	paint_avatars_no_focus_hilite();
	paint_console();
	if(!loc)
		paint_party_status();
	paint_menu({0, 0}, 178, 121);
	caret = {6, 6};
	width = 165;
	height = texth() + 3;
	cancel_position = {6, 104};
}

void paint_small_menu() {
	paint_background(PLAYFLD, 0);
	paint_compass(party.d);
	if(loc)
		paint_dungeon();
	else
		paint_picture();
	paint_avatars_no_focus_hilite();
	paint_console();
	paint_small_menu({68, 124}, 110, 50);
	caret = {71, 126};
	width = 109;
	height = 6 + 1;
	cancel_position = {71, 168};
}

void paint_city() {
	paint_background(PLAYFLD, 0);
	paint_picture();
	paint_party_status();
	paint_party_sheets();
	update_focus_player();
	console_scroll(3000);
	paint_console();
}

void paint_adventure() {
	paint_background(PLAYFLD, 0);
	paint_compass(party.d);
	animation_update();
	paint_dungeon();
	paint_party_sheets();
	update_focus_player();
	console_scroll(3000);
	paint_console();
}

void paint_test_mode() {
	paint_background(PLAYFLD, 0);
	paint_compass(party.d);
	paint_avatars_no_focus_hilite();
}

static void paint_adventure_no_update() {
	paint_background(PLAYFLD, 0);
	paint_compass(party.d);
	paint_dungeon();
	paint_avatars_no_focus();
	console_scroll(3000);
	paint_console();
}

void paint_main_menu() {
	paint_background(MENU, 0);
	caret = {80, 110};
	width = 166;
	height = texth();
}

void fix_animate() {
	if(!need_update_animation)
		return;
	animate_counter++;
	if(loc)
		paint_adventure_no_update();
	else
		paint_city();
	doredraw();
	waitcputime(animation_step);
	memset(disp_damage, 0, sizeof(disp_damage));
	memset(disp_weapon, 0, sizeof(disp_weapon));
	fix_monster_damage_end();
	need_update_animation = false;
}

static void paint_title(const char* title) {
	if(!title)
		return;
	text(title, -1, TextBold);
	if(font == gres(FONT8))
		caret.y += texth() + 4;
	else
		caret.y += texth() + 1;
}

static void paint_blue_title(const char* title) {
	if(!title)
		return;
	auto push_fore = fore;
	fore = colors::title;
	texta(title, TextBold);
	caret.y += texth() + 2;
	fore = push_fore;
}

static void paint_sprites(resid id, point offset, int& focus, int per_line) {
	auto p = gres(id);
	if(!p)
		return;
	auto index = 0;
	auto push_line = caret;
	auto count = per_line;
	while(index < p->count) {
		image(p, index, 0);
		if(focus == index) {
			auto push_caret = caret;
			caret = caret - offset;
			rectb();
			caret = push_caret;
		}
		index++;
		caret.x += width;
		if((--count) == 0) {
			count = per_line;
			caret.y += height;
			caret.x = push_line.x;
		}
		if((caret.y + height) > getheight())
			break;
	}
}

static void show_sprites(resid id, point start, point size) {
	rectpush push;
	auto push_fore = fore;
	auto push_font = font;
	set_small_font();
	int focus = 0;
	auto maximum = gres(id)->count;
	auto per_line = 320 / size.x;
	while(ismodal()) {
		if(focus < 0)
			focus = 0;
		else if(focus > maximum - 1)
			focus = maximum - 1;
		fore = colors::black;
		rectf();
		width = size.x;
		height = size.y;
		caret = start;
		fore = colors::white;
		paint_sprites(id, start, focus, per_line);
		caret = {0, 192};
		text(str("index %1i", focus), -1, TextBold);
		domodal();
		switch(hot.key) {
		case KeyRight: focus++; break;
		case KeyLeft: focus--; break;
		case KeyDown: focus += per_line; break;
		case KeyUp: focus -= per_line; break;
		case KeyEscape: breakmodal(0); break;
		}
		focus_input();
	}
	font = push_font;
	fore = push_fore;
}

void draw_glyph_zoomed(int sym, int zoom);

static void show_scene_font() {
	auto push_font = font;
	answer_index = 0;
	answer_per_page = 128;
	answer_origin = 0;
	auto sx = 10;
	auto size = 8;
	set_small_font();
	while(ismodal()) {
		if(answer_index < 0)
			answer_index = 0;
		else if(answer_index > 127)
			answer_index = 127;
		fore = colors::black;
		rectf();
		width = sx; height = sx;
		correct_answers(256);
		for(auto y = 0; y < 8; y++) {
			for(auto x = 0; x < 16; x++) {
				caret.x = x * width;
				caret.y = y * height;
				auto index = y * 16 + x;
				if(answer_index == index) {
					fore = colors::gray.mix(colors::black, 128);
					rectf();
				}
				fore = colors::white;
				caret.x += (width - size) / 2;
				caret.y += (height - size) / 2;
				glyph(index, 0);
			}
		}
		caret.x = 176;
		caret.y = 16;
		draw_glyph_zoomed(answer_index, 4);
		domodal();
		switch(hot.key) {
		case KeyRight: answer_index++; break;
		case KeyLeft: answer_index--; break;
		case KeyDown: answer_index += 16; break;
		case KeyUp: answer_index -= 16; break;
		case KeyEscape: breakmodal(0); break;
		}
		focus_input();
	}
	font = push_font;
}

static void show_scene_images() {
	static resid sprites[] = {ADVENTURE, BUILDNGS, DUNGEONS, NPC, SCENES};
	static int sprites_count = lenghtof(sprites);
	rectpush push;
	auto push_fore = fore;
	auto push_font = font;
	set_small_font();
	int focus = 0;
	int sprite_number = 0;
	while(ismodal()) {
		if(sprite_number < 0)
			sprite_number = 0;
		else if(sprite_number > sprites_count - 1)
			sprite_number = sprites_count - 1;
		auto pi = bsdata<residi>::elements + sprites[sprite_number];
		auto ps = pi->get();
		auto maximum = ps->count;
		if(focus < 0)
			focus = 0;
		else if(focus > maximum - 1)
			focus = maximum - 1;
		paint_background(PLAYFLD, 0);
		paint_compass(party.d);
		paint_avatars_no_focus_hilite();
		image(0, 0, gres(BORDER), 0, 0);
		image(8, 8, ps, focus, 0);
		caret = {5, 180};
		text(str("sprite %1 index %2i/%3i", pi->id, focus, maximum), -1, TextBold);
		domodal();
		switch(hot.key) {
		case KeyRight: focus++; break;
		case KeyLeft: focus--; break;
		case KeyDown: sprite_number++; break;
		case KeyUp: sprite_number--; break;
		case KeyEscape: breakmodal(0); break;
		}
		focus_input();
	}
	font = push_font;
	fore = push_fore;
}

static int get_file_number(const char* url, const char* mask) {
	char result[260]; stringbuilder sb(result); sb.clear();
	for(io::file::find file(url); file; file.next()) {
		auto pn = file.name();
		if(pn[0] == '.')
			continue;
		if(!szpmatch(pn, mask))
			continue;
		if(!result[0] || szcmp(pn, result) > 0) {
			sb.clear();
			sb.add(pn);
		}
	}
	auto index = 1;
	if(result[0]) {
		char temp[260]; szfnamewe(temp, result);
		auto p = temp + zlen(temp);
		while(p > temp) {
			if(!isnum(p[-1]))
				break;
			p--;
		}
		psnum(p, index);
		index++;
	}
	return index;
}

static void make_screenshoot() {
	auto index = get_file_number("screenshoots", "scr*.bmp");
	char temp[260]; stringbuilder sb(temp);
	sb.add("screenshoots/scr%1.5i.bmp", index);
//	draw::write(temp,
//		draw::canvas->ptr(0, 0), canvas->width, canvas->height, canvas->bpp, canvas->scanline, 0);
}

static void common_input() {
	switch(hot.key) {
	case Ctrl + F5: make_screenshoot(); break;
	}
#ifdef _DEBUG
	switch(hot.key) {
	case Ctrl + 'A': show_sprites(PORTM, {0, 0}, {32, 32}); break;
	case Ctrl + 'S': show_sprites(ITEMGS, {16, 16}, {32, 32}); break;
	case Ctrl + 'D': show_dungeon_images(); break;
	case Ctrl + 'I': show_sprites(ITEMS, {8, 8}, {16, 16}); break;
	case Ctrl + 'L': show_sprites(ITEMGL, {32, 24}, {64, 32}); break;
	case Ctrl + 'P': show_scene_images(); break;
	case Ctrl + 'F': show_scene_font(); break;
	}
#endif
}

static bool can_place(const creaturei* player, wearn id, item* pi) {
	if(id >= Head && id <= LastBelt) {
		if(*pi && !pi->isallow(id))
			return false;
		if(player->wears[id] && !can_remove((item*)player->wears + id))
			return false;
		if(!player->isallow(*pi))
			return false;
	}
	return true;
}

static void update_player(creaturei* p1) {
	auto push_player = player;
	player = p1; update_player();
	player = push_player;
}

void pick_up_dungeon_item();

void pick_up_item() {
	if(!current_select) {
		if(!current_focus)
			return;
		if(*((int*)current_focus) == 0) {
			pick_up_dungeon_item();
			return;
		}
		current_select = current_focus;
	} else {
		auto p1 = (item*)current_select;
		auto p2 = (item*)current_focus;
		current_select = 0;
		auto c1 = item_owner(p1);
		if(!c1)
			return;
		auto c2 = item_owner(p2);
		if(!c2)
			return;
		auto w1 = item_wear(p1);
		auto w2 = item_wear(p2);
		if(!can_place(c1, w1, p2))
			return;
		if(!can_place(c1, w2, p2))
			return;
		if(!can_place(c2, w2, p1))
			return;
		if(!can_place(c2, w1, p1))
			return;
		if(!p2->join(*p1))
			iswap(*p1, *p2);
		update_player(c1);
		if(c1 != c2)
			update_player(c2);
	}
}

static void examine_item() {
	auto pi = (item*)current_focus;
	auto pc = item_owner(pi);
	if(!pc)
		return;
	if(!pc->isactable())
		return;
	if(!(*pi))
		pc->say(getnm("WrongItem"));
	else {
		if(pi->isidentified())
			pc->say(getnm("ExamineIdentifiedItem"), pi->getname());
		else
			pc->say(getnm("ExamineItem"), pi->getname());
	}
}

static void choose_character(int index) {
	auto p = characters[index];
	if(p) {
		player = p;
		if(((creaturei*)current_focus) >= player && ((creaturei*)current_focus) < (player + 1))
			return;
		set_focus_by_player();
	}
}

static bool character_input() {
	switch(hot.key) {
	case '1': case '2': case '3':
	case '4': case '5': case '6':
		choose_character(hot.key - '1');
		break;
	case 'I': switch_page(paint_inventory); break;
	case 'C': switch_page(paint_sheet); break;
	case 'G': switch_page(paint_quest_goals); break;
	case 'X': switch_page(paint_skills); break;
#ifdef _DEBUG
		// Feats do not shown. Player must do not know what feat it is.
		// So only in debug mode.
	case 'F': switch_page(paint_feats); break;
#endif
	case 'O': replace_character(); break;
	case 'P': pick_up_item(); break;
	case 'Q': examine_item(); break;
	case KeyEscape:
		if(!character_view_proc)
			return false;
		clear_page();
		break;
	default:
		return false;
	}
	return true;
}

bool alternate_focus_input() {
	switch(hot.key) {
	case 'A': apply_focus(KeyLeft); break;
	case 'S': apply_focus(KeyRight); break;
	case 'W': apply_focus(KeyUp); break;
	case 'Z': apply_focus(KeyDown); break;
	default: return false;
	}
	return true;
}

void clear_input() {
	pressed_focus = 0;
	hot.key = 0;
}

bool hotkey_input(const hotkeyi* hotkeys) {
	for(auto p = hotkeys; *p; p++) {
		if(hot.key != p->key)
			continue;
		clear_input();
		p->proc();
		return true;
	}
	return false;
}

void city_input(const hotkeyi* hotkeys) {
	if(focus_input())
		return;
	if(alternate_focus_input())
		return;
	if(character_input())
		return;
	if(hotkey_input(hotkeys))
		return;
}

bool adventure_input(const hotkeyi* hotkeys) {
	if(alternate_focus_input())
		return true;
	if(character_input())
		return true;
	if(hotkey_input(hotkeys))
		return true;
	return false;
}

static bool answer_input() {
	int answer_result;
	switch(hot.key) {
	case KeyUp:
	case 'W':
		if(!answer_origin)
			return false;
		answer_result = an.indexof(current_focus);
		if(answer_result != answer_origin)
			return false;
		answer_origin--;
		current_focus = (void*)&an.elements.data[answer_origin];
		break;
	case KeyDown:
	case 'Z':
		if(answer_per_page == -1)
			return false;
		answer_result = an.indexof(current_focus);
		if(answer_result != (answer_origin + answer_per_page - 1))
			return false;
		if(answer_result == (an.getcount() - 1))
			return false;
		answer_origin++;
		current_focus = (void*)&an.elements.data[answer_origin + answer_per_page - 1];
		break;
	default:
		return false;
	}
	return true;
}

static void* answer_get_result(const char* cancel) {
	auto result = (void*)getresult();
	if(an.have(result))
		result = (void*)an.elements[an.indexof(result)].value;
	else if(result == cancel)
		return 0;
	return result;
}

static void* choose_answer(const char* title, const char* cancel, fnevent before_paint, fnanswer answer_paint, int padding, int per_page, fnoutput header_paint) {
	if(!show_interactive)
		return an.random();
	rectpush push;
	pushscene push_scene;
	auto push_origin = answer_origin;
	answer_origin = 0;
	answer_per_page = per_page;
	if(!header_paint)
		header_paint = paint_title;
	while(ismodal()) {
		if(before_paint)
			before_paint();
		header_paint(title);
		paint_answers(answer_paint, update_buttonparam, height + padding);
		if(cancel) {
			if(cancel_position) {
				if(per_page == -1)
					width = textw(cancel) + 6;
				caret = cancel_position;
			}
			answer_paint(1000, 0, cancel, KeyEscape, update_buttonparam);
		}
		domodal();
		if(answer_input())
			continue;
		if(focus_input())
			continue;
		if(alternate_focus_input())
			continue;
		common_input();
	}
	answer_origin = push_origin;
	return answer_get_result(cancel);
}

void* choose_large_menu(const char* header, const char* cancel) {
	return choose_answer(header, cancel, paint_city_menu, button_label, 1, -1, 0);
}

void* choose_small_menu(const char* header, const char* cancel) {
	int maximum = 6;
	if(cancel)
		maximum--;
	return choose_answer(header, cancel, paint_small_menu, text_label_menu, 0, maximum, header_yellow);
}

void* choose_small_menu_spells(const char* header, const char* cancel) {
	int maximum = 6;
	if(cancel)
		maximum--;
	last_spell_level = 1;
	// last_spell_level = 1;
	return choose_answer(header, cancel, paint_small_menu, text_label_menu, 0, maximum, header_yellow_spells);
}

void* choose_main_menu() {
	return choose_answer(0, 0, paint_main_menu, text_label, 1, -1, 0);
}

static int get_total_use(char* source_value) {
	auto result = 0;
	for(auto& e : an.elements) {
		auto index = getbsi((spelli*)e.value);
		result += source_value[index];
	}
	return result;
}

void choose_spells(const char* title, const char* cancel, int spell_type) {
	rectpush push;
	pushscene push_scene;
	auto level = 0;
	auto last_level = level;
	auto spells_known = get_spells_known(player);
	if(!spells_known)
		return;
	add_spells(spell_type, level + 1, spells_known);
	if(an)
		current_focus = (void*)an.elements[0].value;
	while(ismodal()) {
		if(level != last_level) {
			last_level = level;
			add_spells(spell_type, level + 1, spells_known);
			current_focus = bsdata<abilityi>::elements + Spell1 + level;
		}
		auto available_spells = player->abilities[Spell1 + level];
		auto source_value = get_spells_prepared(player);
		auto total_use = get_total_use(source_value);
		paint_background(PLAYFLD, 0);
		paint_avatars_no_focus_hilite();
		paint_console();
		paint_menu({0, 0}, 178, 174);
		caret = {6, 6};
		width = 165;
		height = texth() + 3;
		paint_blue_title(title);
		width = 16;
		auto push_caret = caret;
		if(current_focus >= bsdata<abilityi>::elements + Spell1 && current_focus <= bsdata<abilityi>::elements + Spell9) {
			auto new_level = (abilityi*)current_focus - (bsdata<abilityi>::elements + Spell1);
			if(new_level != level)
				execute(cbsetint, new_level, 0, &level);
		}
		for(int i = 0; i < 9; i++) {
			if(paint_button(str("%1i", i + 1), bsdata<abilityi>::elements + Spell1 + i, '1' + i, TextBold, level == i))
				execute(cbsetint, i, 0, &level);
			caret.x += width + 2;
		}
		caret = push_caret;
		caret.y += texth() + 8;
		width = 165;
		if(!an)
			paint_blue_title(getnm("NoSpellsAvailable"));
		else
			paint_blue_title(str(getnm("SpellsAvailable"), total_use, available_spells));
		caret.y += 2;
		auto index = 0;
		auto current_spell_index = -1;
		for(auto& e : an.elements) {
			auto spell_index = getbsi((spelli*)e.value);
			text_label_left(index, e.value, e.text, e.key, update_buttonparam);
			if(e.value == current_focus)
				current_spell_index = spell_index;
			label_control(str("%1i", source_value[spell_index]), e.value, TextBold | AlignRight);
			caret.y += texth() + 1;
			index++;
		}
		if(cancel) {
			width = textw(cancel) + 6;
			caret = {6, 158};
			button_label(1000, 0, cancel, KeyEscape, update_buttonparam);
		}
		domodal();
		switch(hot.key) {
		case KeyUp:
			if(an.elements && an.elements[0].value == current_focus)
				current_focus = bsdata<abilityi>::elements + level + Spell1;
			else
				apply_focus(hot.key);
			break;
		case KeyDown:
			apply_focus(hot.key);
			break;
		case KeyRight:
			if(current_spell_index != -1 && total_use < available_spells)
				source_value[current_spell_index]++;
			else if(an.findvalue(current_focus) == -1)
				apply_focus(hot.key);
			break;
		case KeyLeft:
			if(current_spell_index != -1 && source_value[current_spell_index] > 0)
				source_value[current_spell_index]--;
			else if(an.findvalue(current_focus) == -1)
				apply_focus(hot.key);
			break;
		}
		common_input();
	}
}

void show_scene(fnevent before_paint, fnevent input, void* focus) {
	rectpush push;
	pushscene push_scene;
	current_focus = focus;
	while(ismodal()) {
		before_paint();
		domodal();
		if(input)
			input();
		common_input();
	}
}

static int menu_button_width() {
	auto result = 0;
	if(!an)
		return 0;
	for(auto& e : an.elements)
		result += textw(e.text) + 6 + 2;
	return result - 2;
}

static void menu_position(const char* format, point& origin, point& size, int padding, int button_area) {
	rect rc = {0, 0, 240, 0};
	textw(rc, format);
	size.x = rc.width() + padding * 2;
	size.y = rc.height() + padding * 2 + button_area;
	origin.x = (getwidth() - size.x) / 2;
	origin.y = (140 - size.y - button_area) / 2;
}

void* choose_dialog(const char* title, int padding) {
	auto push_fore = fore;
	rectpush push;
	pushscene push_scene;
	point origin, size;
	caret = {0, 0};
	width = 320; height = 200;
	paint_blend(colors::black, 16);
	fore = colors::white;
	auto button_area = texth() + 4 + 2;
	menu_position(title, origin, size, padding, button_area);
	auto total_width = menu_button_width();
	while(ismodal()) {
		paint_menu(origin, size.x, size.y);
		caret = origin;
		width = size.x; height = size.y;
		setoffset(padding, padding);
		texta(title, AlignCenter | TextBold);
		caret.y += size.y - button_area - padding * 2 + 4;
		height = button_area - 3;
		caret.x = (320 - total_width) / 2;
		for(auto& e : an.elements) {
			width = textw(e.text) + 6;
			if(paint_button(e.text, e.value, e.key, TextBold))
				execute(buttonparam, (long)e.value);
			caret.x += width;
			caret.x += 2;
		}
		domodal();
		focus_input();
	}
	fore = push_fore;
	return (void*)getresult();
}

static void paint_header(const char* header, bool hilite = false) {
	auto push_fore = fore;
	if(hilite)
		fore = colors::title;
	auto h = texth(header, width);
	texta(header, TextBold);
	caret.y += h;
	fore = push_fore;
}

static void paint_generate_avatars(creaturei* hilite, creaturei** player_position) {
	rectpush push;
	auto push_player = player;
	for(auto i = 0; i < 4; i++) {
		caret.x = 17 + push.caret.x + (i % 2) * 64;
		caret.y = 64 + push.caret.y + (i / 2) * 64;
		width = 33;
		height = 34;
		auto button_data = characters + i;
		if(!player_position)
			focusing(button_data);
		player = characters[i];
		if(player)
			paint_avatar();
		caret.x--; caret.y--;
		if(player_position == button_data)
			image(caret.x, caret.y, gres(XSPL), (getcputime() / 100) % 10, 0);
		if(current_focus == button_data) {
			paint_hilite_rect();
			auto run = button_input(button_data, KeyEnter);
			if(run)
				execute(buttonparam, (long)button_data);
		}
		caret.x -= 14; caret.y += 43;
		width = 60; height = 8;
		if(player) {
			auto push_font = font; set_small_font();
			auto push_fore = fore; fore = colors::title;
			texta(player->getname(), AlignCenterCenter);
			fore = push_fore;
			font = push_font;
		}
	}
	player = push_player;
}

void* choose_generate_box(const char* header, const char* footer) {
	rectpush push;
	pushscene push_scene;
	current_focus = player_position;
	auto push_fore = fore;
	while(ismodal()) {
		paint_background(CHARGEN, 0);
		paint_generate_avatars(player, 0);
		caret = {150, 80}; width = 148;
		paint_header(header);
		if(footer) {
			caret.y += 8;
			paint_header(footer);
			caret.x = 25; caret.y = 181;
			button(CHARGENB, 4, 5, -1, 'P', buttoncancel);
		}
		domodal();
		if(!focus_input())
			common_input();
	}
	fore = push_fore;
	return (void*)getresult();
}

void* choose_generate_box(fnevent proc) {
	rectpush push;
	pushscene push_scene;
	auto push_fore = fore;
	auto push_origin = answer_origin;
	answer_per_page = 4;
	answer_index = 0;
	while(ismodal()) {
		paint_background(CHARGEN, 0);
		paint_generate_avatars(player, player_position);
		caret.x = 144; caret.y = 66; width = 160;
		proc();
		domodal();
		if(!focus_input())
			common_input();
	}
	answer_origin = push_origin;
	fore = push_fore;
	return (void*)getresult();
}

static void paint_avatar_list() {
	int maximum = character_avatars.size();
	correct_answers(maximum);
	rectpush push;
	width = 32; height = 32;
	for(auto i = 0; i < answer_per_page; i++) {
		auto index = answer_origin + i;
		if(index >= maximum)
			break;
		auto n = character_avatars[index];
		image(gres(PORTM), n, 0);
		if(index == answer_index)
			paint_hilite_rect();
		caret.x += 32;
	}
}

static void paint_ability(abilityn i, int header_width) {
	auto caret_x = caret.x;
	auto value = player->get(i);
	auto name = getnm(ids(bsdata<abilityi>::elements[i].id, "Short"));
	text(name, -1, TextBold);
	caret.x += header_width;
	char temp[32]; stringbuilder sb(temp);
	add_value(sb, i);
	text(temp, -1, TextBold);
	caret.y += texth();
	caret.x = caret_x;
}

static void paint_character_info_left() {
	auto push = caret;
	for(auto i = Strenght; i <= Charisma; i = (abilityn)(i + 1)) {
		caret.x = push.x;
		paint_ability(i, 32);
	}
	caret = push;
}

static void paint_character_info_right() {
	auto push = caret;
	const auto w = 28;
	paint_ability(AttackMelee, w);
	paint_ability(AC, w);
	paint_ability(Hits, w);
	caret = push;
}

static void paint_character_info() {
	char temp[260]; stringbuilder sb(temp);
	sb.clear(); sb.add("%Name");
	texta(temp, AlignCenter | TextBold); caret.y += texth();
	sb.clear(); sb.add("%Race %Gender");
	texta(temp, AlignCenter | TextBold); caret.y += texth();
	sb.clear(); sb.add("%Class");
	texta(temp, AlignCenter | TextBold); caret.y += texth() + 4;
	caret.x += 4;
	paint_character_info_left();
	caret.x += 80;
	paint_character_info_right();
}

static void next_sheet_page() {
	static fnevent pages[] = {paint_inventory, paint_sheet, paint_skills, paint_quest_goals};
	int current = -1;
	int maximum = sizeof(pages) / sizeof(pages[0]);
	for(auto i = 0; i < maximum; i++) {
		if(pages[i] == character_view_proc) {
			current = i;
			break;
		}
	}
	if(current == -1)
		current = 0;
	else if(current < maximum - 1)
		current++;
	else
		current = 0;
	character_view_proc = pages[current];
}

void paint_choose_avatars() {
	button(CHARGENB, 2, 3, 8, KeyLeft, cbsetint, answer_index - 1, &answer_index); caret.y += 16;
	button(CHARGENB, 2, 3, 9, KeyRight, cbsetint, answer_index + 1, &answer_index); caret.y += 16;
	caret.x += 33; caret.y -= 32;
	paint_avatar_list();
	caret.x -= 33; caret.y += 36;
	paint_character_info();
	switch(hot.key) {
	case KeyEscape:
		clear_input();
		execute(buttonparam, 0xFF);
		break;
	case KeyEnter:
		clear_input();
		execute(buttonparam, character_avatars[answer_index]);
		break;
	}
}

static void delete_player_posititon() {
	if(!player_position)
		return;
	auto p = *player_position;
	if(!p)
		return;
	p->clear();
	*player_position = 0;
	buttoncancel();
}

static void reroll_player() {
	if(!player_position)
		return;
	auto p = *player_position;
	if(!p)
		return;
	last_gender = player->gender;
	last_race = player->race;
	last_class = player->character_class;
	last_alignment = player->alignment;
	auto avatar = player->avatar;
	player->clear();
	clear_spellbook();
	create_npc(player, 0, is_party_name);
	player->avatar = avatar;
	generate_abilities();
	apply_race_ability();
	roll_player_hits();
	update_player();
	update_player_hits();
	create_player_finish();
}

bool choose_avatar() {
	unsigned char avatars[256];
	auto count = get_avatars(avatars, last_race, last_gender, last_class, no_party_avatars);
	character_avatars = slice<unsigned char>(avatars, count);
	auto result = (unsigned char)(long)choose_generate_box(paint_choose_avatars);
	if(result == 0xFF)
		return false;
	player->avatar = result;
	return true;
}

static void edit_face() {
	choose_avatar();
}

void paint_character_edit() {
	auto push = caret;
	caret.x += 32 * 2 + 1;
	paint_avatar();
	caret.x = push.x; caret.y += 36;
	paint_character_info();
	caret.x = 224; caret.y = 172;
	button(CHARGENB, 6, 7, -1, KeyDelete, delete_player_posititon); caret.x += 41;
	button(CHARGENB, 0, 1, 13, KeyEnter, buttonok);
	caret.x = 224; caret.y = 156;
	button(CHARGENB, 0, 1, 10, 'R', reroll_player); caret.x += 41;
	button(CHARGENB, 0, 1, 12, 'F', edit_face);
}

static void paint_generate_header(const char* header) {
	caret = {146, 70}; width = 156;
	paint_header(header, true);
	caret.x += 8; caret.y += 4; height = texth(); width -= 8;
}

static void paint_generate() {
	paint_background(CHARGEN, 0);
	paint_generate_avatars(player, player_position);
}

void* choose_generate_dialog(const char* header, bool random) {
	an.sort();
	if(random)
		return an.random();
	return choose_answer(header, 0, paint_generate, text_label_left, 2, 10, paint_generate_header);
}

void* show_message(const char* format, bool add_anaswers, const char* cancel, unsigned cancel_key) {
	rectpush push;
	pushscene push_scene;
	auto push_picture = picture;
	while(ismodal()) {
		paint_background(PLAYFLD, 0);
		paint_compass(party.d);
		paint_avatars_no_focus_hilite();
		paint_menu({0, 122}, 319, 77);
		caret = {6, 128};
		width = 308;
		height = 56;
		texta(format, TextBold);
		if(picture)
			paint_picture();
		else if(loc)
			paint_dungeon();
		caret = {4, 184};
		auto index = 0;
		height = texth() + 3;
		if(add_anaswers) {
			for(auto& e : an.elements) {
				width = textw(e.text) + 6;
				button_label(index++, e.value, e.text, e.key, update_buttonparam);
				caret.x += width;
				caret.x += 2;
			}
		}
		if(cancel) {
			width = textw(cancel) + 6;
			button_label(index++, 0, cancel, cancel_key, update_buttonparam);
		}
		domodal();
		if(focus_input())
			continue;
		if(alternate_focus_input())
			continue;
		common_input();
	}
	picture = push_picture;
	return (void*)getresult();
}

bool confirm(const char* format) {
	if(!format)
		return false;
	an.clear();
	an.addv((void*)1, getnm("Yes"), 0, 'Y');
	an.addv((void*)0, getnm("No"), 0, 'N');
	return choose_dialog(format, 8) != 0;
}

void message_box(const char* format) {
	if(!format)
		return;
	an.clear();
	an.addv((void*)1, getnm("OK"), 0, KeyEnter);
	choose_dialog(format, 8);
}

static void main_beforemodal() {
	clear_focus_data();
	cancel_position.clear();
}

void initialize_gui() {
	set_big_font();
	fore = colors::white;
	pbeforemodal = main_beforemodal;
}
