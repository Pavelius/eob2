#include "answers.h"
#include "draw.h"
#include "resid.h"

using namespace draw;

struct eob_fxt {
	short int filesize; // the size of the file
	short int charoffset[128]; // the offset of the pixel data from the beginning of the file, the index is the ascii value
	unsigned char height; // the height of a character in pixel
	unsigned char width; // the width of a character in pixel
	unsigned char data[1]; // the pixel data, one byte per line 
};
namespace colors {
static color main(108, 108, 136);
static color light(148, 148, 172);
static color dark(52, 52, 80);
static color hilite = main.mix(dark, 160);
static color focus(250, 100, 100);
}

static void *current_focus, *pressed_focus;

int draw::texth() {
	if(!font)
		return 0;
	return ((eob_fxt*)font)->height;
}

int draw::textw(int sym) {
	if(!font)
		return 0;
	return ((eob_fxt*)font)->width;
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
	auto f = (eob_fxt*)font;
	int height = f->height;
	int width = f->width;
	for(int h = 0; h < height; h++) {
		unsigned char line = *((unsigned char*)font + ((eob_fxt*)font)->charoffset[sym] + h);
		unsigned char bit = 0x80;
		for(int w = 0; w < width; w++) {
			if((line & bit) == bit)
				pixel(caret.x + w, caret.y + h);
			bit = bit >> 1;
		}
	}
}

static void button_back(bool focused) {
	auto push_fore = fore;
	fore = focused ? colors::hilite : colors::main;
	rectf();
	fore = push_fore;
}

static void border_up() {
	rectpush push;
	fore = colors::dark;
	line(caret.x, caret.y + height);
	line(caret.x + width, caret.y);
	fore = colors::light;
	//draw::line(rc.x2, rc.y1, rc.x2, rc.y2 - 1);
	//draw::line(rc.x1 + 1, rc.y1, rc.x2 - 1, rc.y1);
}

static void border_down() {
	rectpush push;
	fore = colors::light;
	line(caret.x, caret.y + height);
	line(caret.x + width, caret.y);
	fore = colors::light;
}

static void button_frame(int count, bool focused, bool pressed) {
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

static bool button_input(void* button_data, unsigned key) {
	auto isfocused = (current_focus == button_data);
	if((isfocused && hot.key == KeyEnter) || (key && hot.key == key))
		pressed_focus = button_data;
	else if(hot.key == InputKeyUp && pressed_focus == button_data) {
		pressed_focus = 0;
		return true;
	}
	return false;
}

static bool paint_button(const char* title, void* button_data, unsigned key, unsigned flags = TextBold) {
	if(!button_data)
		button_data = (void*)title;
	auto run = button_input(button_data, key);
	button_frame(1, false, pressed_focus == button_data);
	if(current_focus == button_data)
		fore = colors::focus;
	text(title, -1, flags);
	caret.y += height + 3;
	return run;
}

static void center_text_button(int index, const void* data, const char* format, fnevent proc) {
	auto push_fore = fore;
	fore = colors::white;
	texta(format, AlignCenter | TextBold);
	caret.y += texth() + 1;
	fore = push_fore;
}

static void paint_answers(fnanswer paintcell) {
	auto index = 0;
	for(auto& e : an.elements) {
		paintcell(index, e.value, e.text, buttonparam);
		index++;
	}
}

static void paint_background(resid v, int frame) {
	draw::image(gres(v), frame, 0);
}

void* choose_answer(point origin, resid background, int frame, int column_width) {
	rectpush push;
	while(ismodal()) {
		paint_background(background, frame);
		caret = origin;
		width = column_width;
		paint_answers(center_text_button);
		domodal();
		if(hot.key == KeyEscape)
			breakmodal(0);
	}
	return (void*)getresult();
}

void initialize_gui() {
	font = gres(FONT8);
}