#include "answers.h"
#include "creature.h"
#include "class.h"
#include "console.h"
#include "direction.h"
#include "draw.h"
#include "gender.h"
#include "math.h"
#include "picture.h"
#include "race.h"
#include "resid.h"
#include "timer.h"
#include "unit.h"
#include "view_focus.h"

using namespace draw;

struct pushscene : pushfocus {
};
namespace colors {
static color main(108, 108, 136);
static color info(64, 64, 64);
static color light(148, 148, 172);
static color dark(52, 52, 80);
static color hilite = main.mix(dark, 160);
static color focus(250, 100, 100);
static color down(81, 85, 166);
static color form(164, 164, 186);
}

static fnevent character_view_proc;
static void* current_select;

static unsigned get_frame_tick() {
	return getcputime() / 10;
}

static int get_alpha(int base, unsigned range) {
	auto seed = get_frame_tick() % range;
	auto half = range / 2;
	if(seed < half)
		return base * seed / half;
	else
		return base * (range - seed) / half;
}

static bool is_cursor_visible(unsigned range) {
	auto seed = get_frame_tick() % range;
	return seed >= (range / 3);
}

static const char* namesh(const char* id) {
	auto p = getnme(ids(id, "Short"));
	if(!p)
		p = "XXX";
	return p;
}

static void set_small_font() {
	font = gres(FONT6);
}

static void set_big_font() {
	font = gres(FONT8);
}

static void copy_image(point origin, point dest, int w, int h) {
	auto scan_size = sizeof(color) * w;
	for(auto i = 0; i < h; i++)
		memcpy(canvas->ptr(dest.x, dest.y + i), canvas->ptr(origin.x, origin.y + i), scan_size);
}

static void paint_background(resid v, int frame) {
	draw::image(gres(v), frame, 0);
}

static void paint_picture() {
	draw::image(gres(picture.id), picture.frame, 0);
}

static void button_back(bool focused) {
	rectpush push;
	auto push_fore = fore;
	fore = focused ? colors::hilite : colors::main;
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
	setoffset(1, 1);
}

static bool button_input(const void* button_data, unsigned key) {
	auto isfocused = (current_focus == button_data);
	if((isfocused && hot.key == KeyEnter) || (key && hot.key == key))
		pressed_focus = (void*)button_data;
	else if(hot.key == InputKeyUp && pressed_focus == button_data) {
		pressed_focus = 0;
		return true;
	}
	return false;
}

static bool paint_button(const char* title, const void* button_data, unsigned key, unsigned flags = TextBold) {
	rectpush push;
	auto push_fore = fore;
	if(!button_data)
		button_data = (void*)title;
	focusing(button_data);
	auto run = button_input(button_data, key);
	auto pressed = (pressed_focus == button_data);
	button_frame(1, false, pressed);
	if(current_focus == button_data)
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
	auto index = 0;
	for(auto& e : an.elements) {
		paintcell(index, &e, e.text, pushbutton);
		caret.y += height_grid;
		index++;
	}
}

static unsigned get_key(int index) {
	if(index < 9)
		return '1' + index;
	return 0;
}

void text_label(int index, const void* data, const char* format, fnevent proc) {
	auto push_fore = fore;
	focusing(data);
	if(button_input(data, get_key(index)))
		execute(proc, (long)data);
	fore = colors::white;
	if(current_focus == data)
		fore = colors::focus;
	if(pressed_focus == data)
		fore = fore.darken();
	texta(format, AlignCenter | TextBold);
	fore = push_fore;
}

void button_label(int index, const void* data, const char* format, fnevent proc) {
	if(paint_button(format, data, get_key(index)))
		execute(proc, (long)proc);
}

static void update_buttonparam() {
	updatewindow();
	buttonparam();
}

static void set_player_by_focus() {
	auto i = bsdata<creaturei>::source.indexof(current_focus);
	if(i != -1)
		player = bsdata<creaturei>::elements + i;
}

static void switch_page(fnevent proc) {
	if(character_view_proc == proc)
		character_view_proc = 0;
	else {
		set_player_by_focus();
		character_view_proc = proc;
	}
}

static void clear_page() {
	character_view_proc = 0;
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
	auto push_fore = fore;
	fore = colors::white;
	auto i = get_compass_index(d);
	image(114, 132, gres(COMPASS), i, 0);
	image(79, 158, gres(COMPASS), 4 + i, 0);
	image(150, 158, gres(COMPASS), 8 + i, 0);
}

void paint_adventure() {
	paint_background(PLAYFLD, 0);
	paint_compass(Right);
}

static void paint_menu(point position, int object_width, int object_height) {
	rectpush push;
	caret = position;
	width = object_width;
	height = object_height;
	button_frame(2, false, false);
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

static void paint_avatar() {
	if(player->avatar == 0xFF)
		return;
	rectpush push; width = 31; height = 31;
	if(player->isdead())
		image(gres(PORTM), 0, 0);
	else
		image(gres(PORTM), player->avatar, 0);
	// static color blink_colors[] = {colors::green, colors::red, colors::blue, colors::form};
	// blend_avatar(blink_colors);
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

static void paint_select_rect() {
	auto push_fore = fore;
	fore = colors::focus;
	if(current_select == current_focus)
		fore = fore.mix(colors::white);
	rectb();
	fore = push_fore;
}

static void paint_item(item& it, wearn id, int emphty_avatar = -1) {
	rectpush push;
	focusing(&it);
	if(current_select == &it)
		paint_select_rect();
	else if(current_focus == &it)
		paint_focus_rect();
	auto avatar = it.geti().avatar;
	if(!it)
		avatar = emphty_avatar;
	if(avatar != -1)
		image(caret.x + width / 2, caret.y + height / 2, gres(ITEMS), avatar, 0);
}

static void paint_ring(item& it, wearn id) {
	rectpush push;
	width = height = 10;
	paint_item(it, id);
}

static void paint_sheet_head() {
	const point origin = {178, 0};
	caret = origin;
	set_small_font();
	image(gres(INVENT), 0, 0);
	caret.x = origin.x + 4;
	caret.y = origin.y + 4;
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
	greenbar(25, 100);
	caret.x = origin.x + 2;
	caret.y = origin.y + 36;
	width = 140; height = 131;
	fore = colors::info;
}

static void paint_blank() {
	auto push_caret = caret;
	auto push_fore = fore;
	fore = colors::form;
	rectf();
	fore = color(208, 208, 216);
	caret.x = 274; caret.y = 35;
	line(319, caret.y);
	line(319, 166);
	fore = color(88, 88, 116);
	caret.x = 178;
	line(319, caret.y);
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

static void textn(const char* format, int value, const char* value_format = 0) {
	char temp[260]; stringbuilder sb(temp);
	if(!value_format)
		value_format = "%1i";
	sb.add(value_format, value);
	textr(temp);
	text(format);
	caret.y += texth() + 1;
}

static void addv(stringbuilder& sb, const dice& value) {
	sb.add("%1i-%2i", value.minimum(), value.maximum());
}

static void textn(abilityn id) {
	char temp[260]; stringbuilder sb(temp);
	int value = 0;
	switch(id) {
	case AttackMelee:
		sb.add("%1i", 20 - player->get(id));
		break;
	case DamageMelee:
		addv(sb, player->getdamage(RightHand));
		break;
	case AC:
		sb.add("%1i", 10 - player->get(id));
		break;
	case Hits:
		sb.add("%1i", player->hpm);
		break;
	case Speed:
		sb.add("%1i", -player->get(id));
		break;
	case ReactionBonus:
		sb.add("%+1i", player->get(id));
		break;
	default:
		sb.add("%1i", player->get(id));
		break;
	}
	textr(temp);
	textn(namesh(bsdata<abilityi>::elements[id].id));
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
	textn(AttackMelee);
	textn(AC);
	textn(Hits);
	textn(DamageMelee);
	textn(Speed);
	textn(ReactionBonus);
}

static void paint_sheet() {
	rectpush push;
	auto push_font = font;
	auto push_fore = fore;
	paint_sheet_head();
	paint_blank();
	header(getnm("Characterinfo"));
	textn(getnm(bsdata<classi>::elements[player->type].id));
	//text(bsdata<alignmenti>::elements[pc->getalignment()].name); y1 += draw::texth();
	textn(str("%1 %2", bsdata<racei>::elements[player->race].getname(), bsdata<genderi>::elements[player->gender].getname()));
	caret.y += texth();
	width = 88;
	paint_ability();
	caret.x += 6 * 15 + 3;
	width = 44;
	paint_stats();
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
	width = 134;
	for(auto i = (abilityn)SaveVsParalization; i <= DetectSecrets; i = (abilityn)(i + 1)) {
		auto value = player->get(i);
		if(value <= 0)
			continue;
		textn(bsdata<abilityi>::elements[i].getname(), player->get(i), "%1i%%");
	}
	font = push_font;
	fore = push_fore;
}

static void warning(const char* format, unsigned flags) {
	auto push_fore = fore;
	fore = colors::text.mix(colors::red);
	texta(format, flags);
	fore = push_fore;
}

static void paint_states() {
	if(player->isdead())
		warning(getnm("Dead"), AlignCenter);
	else if(player->isdisabled())
		warning(getnm("Disabled"), AlignCenter);
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

static void paint_character() {
	rectpush push;
	auto push_font = font;
	auto push_caret = caret;
	auto push_fore = fore; fore = colors::black;
	set_small_font();
	caret.y = push.caret.y + 3;
	width = 65;
	texta(player->getname(), AlignCenter);
	width = 31;
	height = 16;
	caret.x = push.caret.x + 33;
	caret.y = push.caret.y + 10;
	paint_item(player->wears[RightHand], RightHand, 84);
	caret.y = push.caret.y + 26;
	paint_item(player->wears[LeftHand], LeftHand, 83);
	caret.x = push.caret.x + 2;
	caret.y = push.caret.y + 10;
	width = 31;
	paint_avatar();
	caret.y = push.caret.y + 44;
	width = 65;
	texta(str("%1i of %2i", player->hp, player->hpm), AlignCenter);
	fore = push_fore;
	font = push_font;
	caret = push_caret;
}

static void paint_character(bool disabled) {
	rectpush push;
	auto push_disabled = disable_input;
	width = 65; height = 52;
	paint_character();
	if(disabled)
		paint_disabled();
	disable_input = push_disabled;
}

static void paint_avatars() {
	static point points[] = {{183, 1}, {255, 1}, {183, 53}, {255, 53}, {183, 105}, {255, 105}};
	rectpush push;
	auto push_player = player;
	if(party.units[4])
		copy_image({183, 53}, {183, 105}, 65, 52);
	if(party.units[5])
		copy_image({255, 53}, {255, 105}, 65, 52);
	for(auto i = 0; i < 6; i++) {
		caret = points[i];
		player = party.units[i];
		if(!player)
			continue;
		paint_character(player->isdisabled());
	}
	player = push_player;
}

static void paint_party_sheets() {
	if(character_view_proc)
		character_view_proc();
	else
		paint_avatars();
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

void paint_adventure_menu() {
	paint_background(PLAYFLD, 0);
	paint_party_sheets();
	paint_console();
	paint_menu({0, 0}, 177, 174);
	caret = {6, 6};
	width = 165;
	height = texth() + 3;
}

void paint_main_menu() {
	paint_background(MENU, 0);
	caret = {80, 110};
	width = 166;
	height = texth();
}

static void paint_title(const char* title) {
	if(!title)
		return;
	auto push_fore = fore;
	text(title, -1, TextBold);
	caret.y += texth() + 4;
	fore = push_fore;
}

static void paint_sprites(resid id, point offset, int& focus, int per_line) {
	auto p = gres(id);
	if(!p)
		return;
	auto index = 0;
	auto push_caret = caret;
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

static void debug_input() {
#ifdef _DEBUG
	switch(hot.key) {
	case Ctrl + 'I': show_sprites(ITEMS, {8, 8}, {16, 16}); break;
	case Ctrl + 'A': show_sprites(PORTM, {0, 0}, {32, 32}); break;
	}
#endif
}

static bool can_place(const creaturei* player, wearn id, item* pi) {
	if(id >= Head && id <= Quiver) {
		if(*pi && !pi->isallow(id))
			return false;
		if(player->wears[id] && !player->isremove(player->wears + id))
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

static bool can_remove(item* pi) {
	auto player = item_owner(pi);
	if(!player)
		return true;
	auto w = item_wear(pi);
	if(w >= Head && w <= Quiver) {
		// TODO: check if cursed
	}
	return true;
}

static void pick_up_item() {
	if(!current_select) {
		if(!current_focus)
			return;
		if(*((int*)current_focus) == 0)
			return;
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
		if(!can_place(c2, w2, p1))
			return;
		iswap(*p1, *p2);
		update_player(c1);
		update_player(c2);
	}
}

static void examine_item() {
	auto pi = (item*)current_focus;
	auto pc = item_owner(pi);
	if(!pc)
		return;
	if(!(*pi))
		pc->say(getnm("WrongItem"));
	else
		pc->say(getnm("ExamineItem"), pi->getname());
}

static void character_input() {
	switch(hot.key) {
	case 'I': switch_page(paint_inventory); break;
	case 'C': switch_page(paint_sheet); break;
	case 'X': switch_page(paint_skills); break;
	case 'P': pick_up_item(); break;
	case 'Q': examine_item(); break;
	case KeyEscape:
		if(character_view_proc)
			clear_page();
		break;
	}
}

static void alternate_focus_input() {
	switch(hot.key) {
	case 'A': apply_focus(KeyLeft); break;
	case 'S': apply_focus(KeyRight); break;
	case 'W': apply_focus(KeyUp); break;
	case 'Z': apply_focus(KeyDown); break;
	default: return;
	}
}

void* choose_answer(const char* title, fnevent before_paint, fnanswer answer_paint, int padding) {
	if(!show_interactive)
		return an.random();
	rectpush push;
	pushscene push_scene;
	while(ismodal()) {
		if(before_paint)
			before_paint();
		paint_title(title);
		paint_answers(answer_paint, update_buttonparam, height + padding);
		domodal();
		focus_input();
		alternate_focus_input();
		debug_input();
		character_input();
	}
	return (void*)getresult();
}

static void main_beforemodal() {
	clear_focus_data();
}

void initialize_gui() {
	set_big_font();
	fore = colors::white;
	pbeforemodal = main_beforemodal;
}