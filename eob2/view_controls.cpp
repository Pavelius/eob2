#include "answers.h"
#include "boost.h"
#include "creature.h"
#include "class.h"
#include "console.h"
#include "direction.h"
#include "draw.h"
#include "dungeon.h"
#include "hotkey.h"
#include "gender.h"
#include "location.h"
#include "math.h"
#include "picture.h"
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
static int answer_origin, answer_per_page;
bool need_update_animation;
unsigned long current_cpu_time;

struct pushscene : pushfocus {
	const sprite*	font;
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

void set_small_font() {
	font = gres(FONT6);
}

static void set_big_font() {
	font = gres(FONT8);
}

static int get_party_disp(creaturei* target, wearn id) {
	if(!target)
		return 0;
	int pind = get_party_index(target);
	if(pind == -1)
		return 0;
	if(id == RightHand)
		return disp_weapon[pind][0];
	else if(id == LeftHand)
		return disp_weapon[pind][1];
	return 0;
}

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
	if(picture)
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
	auto isfocused = (current_focus == button_data);
	if((isfocused && (hot.key == KeyEnter || hot.key == key_hot)) || (key && hot.key == key))
		pressed_focus = (void*)button_data;
	else if(hot.key == InputKeyUp && pressed_focus == button_data) {
		pressed_focus = 0;
		return true;
	}
	return false;
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
	if(hits == -1)
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
		// paintcell(index, e.value, e.text, e.key, pushbutton);
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
		else if(answer_per_page != -1 && index == (answer_origin + answer_per_page - 1) && (answer_origin + answer_per_page < an.getcount()))
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

static void set_focus_by_player() {
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
	blend_avatar(avatar_colors, 32);
}

static void paint_avatar_border() {
	adat<color, 8> avatar_colors;
	referencei target = player;
	for(auto& e : bsdata<boosti>()) {
		if(e.target != target)
			continue;
		if(e.effect.iskind<spelli>()) {
			auto ps = bsdata<spelli>::elements + e.effect.value;
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

static void paint_avatar() {
	if(player->avatar == 0xFF)
		return;
	rectpush push; width = 31; height = 31;
	if(player->isdead())
		image(gres(PORTM), 0, 0);
	else
		image(gres(PORTM), player->avatar, 0);
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
	auto push_fore = fore;
	fore = colors::white.mix(colors::black, get_alpha(255, 160));
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
		caret.x = push.caret.x + push.width - textw(ps) - 1;
		auto push_fore = fore;
		fore = colors::white;
		text(ps);
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
	int bonus;
	switch(id) {
	case AttackMelee:
		sb.add("%1i", 20 - player->get(id));
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

void header_yellow(const char* format) {
	auto push_fore = fore;
	fore = colors::yellow;
	text(format, -1);
	caret.y += texth() + 1;
	fore = push_fore;
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
	auto pc = bsdata<classi>::elements + player->type;
	auto exp = player->experience / pc->count;
	for(int i = 0; i < pc->count; i++) {
		auto m = pc->classes[i];
		caret.x = push.caret.x;
		text(bsdata<classi>::elements[pc->classes[i]].getname());
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
	textn(getnm(bsdata<classi>::elements[player->type].id));
	textn(bsdata<alignmenti>::elements[player->alignment].getname());
	textn(str("%1 %2", bsdata<racei>::elements[player->race].getname(), bsdata<genderi>::elements[player->gender].getname()));
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
	texta(str("%1i of %2i", player->hp, player->hpm), AlignCenter);
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
	//if(!current_focus)
	//	set_focus_by_player();
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
		paint_character(player->isdisabled(), hilite_player && (player == push_player));
	}
	player = push_player;
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

static void field(const char* header, int title_width, int total, int value, int maximum) {
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
	texta(bsdata<locationi>::elements[party.location].getname(), AlignCenter, colors::yellow);
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
	paint_small_menu({70, 124}, 108, 50);
	caret = {73, 126};
	width = 107;
	height = 6 + 1;
	cancel_position = {73, 167};
}

void paint_city() {
	paint_background(PLAYFLD, 0);
	paint_picture();
	paint_party_status();
	paint_party_sheets();
	update_focus_player();
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

static void debug_input() {
#ifdef _DEBUG
	switch(hot.key) {
	case Ctrl + 'I': show_sprites(ITEMS, {8, 8}, {16, 16}); break;
	case Ctrl + 'A': show_sprites(PORTM, {0, 0}, {32, 32}); break;
	case Ctrl + 'S': show_sprites(ITEMGS, {16, 16}, {32, 32}); break;
	case Ctrl + 'L': show_sprites(ITEMGL, {32, 24}, {64, 32}); break;
	}
#endif
}

static bool can_place(const creaturei* player, wearn id, item* pi) {
	if(id >= Head && id <= Quiver) {
		if(*pi && !pi->isallow(id))
			return false;
		if(player->wears[id] && !can_remove(player->wears + id))
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

static void pick_up_item() {
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
		if(!can_place(c2, w2, p1))
			return;
		if(!p2->join(*p1))
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

bool character_input() {
	switch(hot.key) {
	case '1': case '2': case '3':
	case '4': case '5': case '6':
		choose_character(hot.key - '1');
		break;
	case 'I': switch_page(paint_inventory); break;
	case 'C': switch_page(paint_sheet); break;
	case 'X': switch_page(paint_skills); break;
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

void alternate_focus_input() {
	switch(hot.key) {
	case 'A': apply_focus(KeyLeft); break;
	case 'S': apply_focus(KeyRight); break;
	case 'W': apply_focus(KeyUp); break;
	case 'Z': apply_focus(KeyDown); break;
	}
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
	focus_input();
	alternate_focus_input();
	if(character_input())
		return;
	if(hotkey_input(hotkeys))
		return;
}

bool adventure_input(const hotkeyi* hotkeys) {
	alternate_focus_input();
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
		answer_result = an.findvalue(current_focus);
		if(answer_result != answer_origin)
			return false;
		answer_origin--;
		current_focus = (void*)an.elements[answer_origin].value;
		break;
	case KeyDown:
	case 'Z':
		if(answer_per_page == -1)
			return false;
		answer_result = an.findvalue(current_focus);
		if(answer_result != (answer_origin + answer_per_page - 1))
			return false;
		if(answer_result == (an.getcount() - 1))
			return false;
		answer_origin++;
		current_focus = (void*)an.elements[answer_origin + answer_per_page - 1].value;
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
		if(!answer_input()) {
			focus_input();
			alternate_focus_input();
		}
		debug_input();
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
		debug_input();
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
		debug_input();
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

void* show_message(const char* format, bool add_anaswers, const char* cancel, unsigned cancel_key) {
	rectpush push;
	pushscene push_scene;
	auto push_picture = picture;
	while(ismodal()) {
		paint_background(PLAYFLD, 0);
		paint_avatars_no_focus_hilite();
		paint_console();
		paint_menu({0, 122}, 319, 77);
		caret = {6, 128};
		width = 308;
		height = 56;
		// rectb();
		texta(format, TextBold);
		paint_picture();
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
		focus_input();
		alternate_focus_input();
		debug_input();
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
