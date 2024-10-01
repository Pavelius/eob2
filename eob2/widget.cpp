#include "answers.h"
#include "direction.h"
#include "draw.h"
#include "resid.h"
#include "view_focus.h"

using namespace draw;

namespace {
struct pushscene : pushfocus {
};
struct renderi {
	void* av;
	rect rc;
	void clear() { memset(this, 0, sizeof(*this)); }
};
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

static renderi render_objects[48];
static renderi*	render_current;

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

void* choose_answer(point origin, resid background, int frame, int column_width) {
	if(!show_interactive)
		return an.random();
	rectpush push;
	pushscene push_scene;
	while(ismodal()) {
		paint_background(background, frame);
		caret = origin;
		width = column_width;
		height = texth();
		paint_answers(text_label, buttonparam, height);
		domodal();
		if(hot.key == KeyEscape)
			breakmodal(0);
		else
			focus_input();
	}
	return (void*)getresult();
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

void paint_adventure_menu() {
	paint_background(PLAYFLD, 0);
	copy_image({183, 53}, {183, 105}, 65, 52);
	copy_image({255, 53}, {255, 105}, 65, 52);
}

static void paint_title(const char* title) {
	if(!title)
		return;
	auto push_fore = fore;
	text(title, -1, TextBold);
	caret.y += texth() + 4;
	fore = push_fore;
}

void* choose_answer(const char* title, fnevent before_paint) {
	if(!show_interactive)
		return an.random();
	const auto column_width = 166;
	rectpush push;
	pushscene push_scene;
	while(ismodal()) {
		if(before_paint)
			before_paint();
		paint_menu({0, 0}, 178, 174);
		caret = {6, 6};
		paint_title(title);
		width = column_width;
		height = texth() + 3;
		paint_answers(button_label, buttonparam, height + 2);
		domodal();
		focus_input();
	}
	return (void*)getresult();
}

void initialize_gui() {
	set_big_font();
	fore = colors::white;
}