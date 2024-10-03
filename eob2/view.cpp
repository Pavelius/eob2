#include "answers.h"
#include "creature.h"
#include "direction.h"
#include "draw.h"
#include "picture.h"
#include "resid.h"
#include "timer.h"
#include "unit.h"
#include "view_focus.h"

using namespace draw;

struct pushscene : pushfocus {
};
namespace {
struct fxt {
	short int filesize; // the size of the file
	short int charoffset[128]; // the offset of the pixel data from the beginning of the file, the index is the ascii value
	unsigned char height; // the height of a character in pixel
	unsigned char width; // the width of a character in pixel
	unsigned char data[1]; // the pixel data, one byte per line 
};
}
namespace colors {
static color main(108, 108, 136);
static color light(148, 148, 172);
static color dark(52, 52, 80);
static color hilite = main.mix(dark, 160);
static color focus(250, 100, 100);
}

int draw::texth() {
	if(!font)
		return 0;
	return ((fxt*)font)->height;
}

int draw::textw(int sym) {
	if(!font)
		return 0;
	return ((fxt*)font)->width;
}

void draw::glyph(int sym, unsigned flags) {
	if(flags & TextBold) {
		auto push_caret = caret;
		auto push_fore = fore;
		fore = colors::black;
		caret.x += 1;
		caret.y += 1;
		glyph(sym, 0);
		fore = push_fore;
		caret = push_caret;
	}
	auto f = (fxt*)font;
	int height = f->height;
	int width = f->width;
	for(int h = 0; h < height; h++) {
		unsigned char line = *((unsigned char*)font + ((fxt*)font)->charoffset[sym] + h);
		unsigned char bit = 0x80;
		for(int w = 0; w < width; w++) {
			if((line & bit) == bit)
				pixel(caret.x + w, caret.y + h);
			bit = bit >> 1;
		}
	}
}

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
	image(gres(PORTM), player->avatar, 0);
	//static color blink_colors[] = {colors::green, colors::red, colors::blue, colors::form};
	//blend_avatar(blink_colors);
}

static void textjf(const char* format, int x, int y, int text_width, unsigned flags) {
	rectpush push;
	caret.x += x;
	caret.y += y;
	width = text_width;
	texta(format, AlignCenter);
}

static void paint_character_name() {
}

static void paint_character() {
	auto push_font = font;
	auto push_caret = caret;
	auto push_fore = fore; fore = colors::black;
	set_small_font();
	textjf(player->getname(), 0, 3, 65, AlignCenter);
	caret.x += 2;
	caret.y += 10;
	paint_avatar();
	caret.x -= 2;
	caret.y += 31;
	textjf(str("%1i of %2i", player->hp, player->hpm), 0, 3, 65, AlignCenter);
	fore = push_fore;
	font = push_font;
	caret = push_caret;
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
		paint_character();
	}
	player = push_player;
}

void paint_adventure_menu() {
	paint_background(PLAYFLD, 0);
	paint_avatars();
	paint_menu({0, 0}, 178, 174);
	caret = {6, 6};
	width = 166;
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

static void paint_sprites(resid id, point offset, int& focus) {
	auto p = gres(id);
	if(!p)
		return;
	auto index = 0;
	auto push_caret = caret;
	auto push_line = caret;
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
		if((caret.x + width) > getwidth()) {
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
	int focus = 0;
	auto maximum = gres(id)->count;
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
		paint_sprites(id, start, focus);
		domodal();
		switch(hot.key) {
		case KeyRight: focus++; break;
		case KeyLeft: focus--; break;
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
		debug_input();
	}
	return (void*)getresult();
}

void initialize_gui() {
	set_big_font();
	fore = colors::white;
}