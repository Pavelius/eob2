#include "answers.h"
#include "draw.h"
#include "resid.h"

using namespace draw;

namespace {
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

static void *current_focus, *pressed_focus;
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

static point center(const rect& rc) {
	return{(short)rc.x1, (short)rc.y1};
}

static void focusing(void* av) {
	if(!av)
		return;
	if(!render_current
		|| render_current >= render_objects + sizeof(render_objects) / sizeof(render_objects[0]))
		render_current = render_objects;
	render_current[0].rc = {caret.x, caret.y, caret.x + width, caret.y + height};
	render_current[0].av = av;
	render_current++;
	render_current->clear();
	if(!current_focus)
		current_focus = av;
}

static renderi* getby(void* av) {
	for(auto& e : render_objects) {
		if(!e.av)
			return 0;
		if(e.av == av)
			return &e;
	}
	return 0;
}

static renderi* getfirst() {
	for(auto& e : render_objects) {
		if(!e.av)
			return 0;
		return &e;
	}
	return 0;
}

static renderi* getlast() {
	auto p = render_objects;
	for(auto& e : render_objects) {
		if(!e.av)
			break;
		p = &e;
	}
	return p;
}

static renderi* next_focus(void* ev, int key) {
	if(!key)
		return 0;
	auto pc = getby(ev);
	if(!pc)
		pc = getfirst();
	if(!pc)
		return 0;
	auto pe = pc;
	auto pl = getlast();
	int inc = 1;
	if(key == KeyLeft || key == KeyUp)
		inc = -1;
	renderi* r1 = 0;
	auto p1 = center(pe->rc);
	while(true) {
		pc += inc;
		if(pc > pl)
			pc = render_objects;
		else if(pc < render_objects)
			pc = pl;
		if(pe == pc) {
			if(r1)
				return r1;
			return pe;
		}
		auto p2 = center(pc->rc);
		renderi* r2 = 0;
		switch(key) {
		case KeyRight:
			if(pe->rc.x2 < pc->rc.x1
				&& ((pe->rc.y1 >= pc->rc.y1 && pe->rc.y1 <= pc->rc.y2)
					|| (pe->rc.y2 >= pc->rc.y1 && pe->rc.y2 <= pc->rc.y2)))
				r2 = pc;
			break;
		case KeyLeft:
			if(pe->rc.x1 > pc->rc.x2
				&& ((pe->rc.y1 >= pc->rc.y1 && pe->rc.y1 <= pc->rc.y2)
					|| (pe->rc.y2 >= pc->rc.y1 && pe->rc.y2 <= pc->rc.y2)))
				r2 = pc;
			break;
		case KeyDown:
			if(p1.y < p2.y)
				r2 = pc;
			break;
		case KeyUp:
			if(p2.y < p1.y)
				r2 = pc;
			break;
		default:
			return pc;
		}
		if(r2) {
			if(!r1)
				r1 = r2;
			else {
				int d1 = distance(p1, center(r1->rc));
				int d2 = distance(p1, center(r2->rc));
				if(d2 < d1)
					r1 = r2;
			}
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
	focusing(button_data);
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

static void paint_answers(fnanswer paintcell, fnevent pushbutton) {
	auto index = 0;
	for(auto& e : an.elements) {
		paintcell(index, e.value, e.text, pushbutton);
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
		paint_answers(center_text_button, buttonparam);
		domodal();
		if(hot.key == KeyEscape)
			breakmodal(0);
	}
	return (void*)getresult();
}

void initialize_gui() {
	set_big_font();
}