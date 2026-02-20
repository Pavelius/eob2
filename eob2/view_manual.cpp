#include "bsdata.h"
#include "draw.h"
#include "stringbuilder.h"
#include "textscript.h"
#include "view.h"
#include "view_focus.h"

using namespace draw;

void addkey(stringbuilder& sb, int key);

static void button_manual(const char* header, unsigned key, fnevent proc) {
	auto push_width = width;
	width = textw(header) + 4 * 2 - 2;
	button_label(0, (void*)proc, header, KeyEscape, proc);
	caret.x += width + 2;
	width = push_width;
}

static void paint_manual_content(const char* content) {
	static int current, maximum;
	static int cashe_origin;
	static const char* cashe_string;
	rectpush push;
	auto push_clip = clipping;
	setcliparea();
	setoffset(4, 4);
	height -= texth() + 8;
	textf(content, cashe_string, cashe_origin, current, maximum);
	clipping = push_clip;
}

static void move_up() {
}

static void move_down() {
}

static void* choose_manual(const char* content) {
	auto push_flags = text_flags;
	text_flags = TextBold;
	pushfocus push;
	while(ismodal()) {
		width = getwidth() - 1; height = getheight() - 1;
		button_frame(2, false, false);
		setoffset(4, 4);
		paint_manual_content(content);
		setpos(caret.x, getheight() - texth() - 8); height = texth() + 3;
		// button_manual("\x5e", KeyUp, move_up);
		// button_manual("\x8b", KeyDown, move_down);
		button_manual(getnm("Cancel"), KeyEscape, buttoncancel);
		domodal();
		focus_input();
	}
	text_flags = push_flags;
	return (void*)getresult();
}

static void add_keybind(stringbuilder& sb, int key, const char* id) {
	sb.add("\n");
	addkey(sb, key);
	sb.add("\t");
	auto pn = getnme(ids(id, "Action"));
	if(!pn)
		pn = getnm(id);
	sb.add(pn);
}

static void add_manual_content(stringbuilder& sb, const char* id) {
	auto pi = bsdata<textscript>::find(ids(id, "Manual"));
	if(pi) {
		pi->proc(sb);
		return;
	}
	sb.addn(getnm(ids(id, "Info")));
}

void choose_manual(const textscript* p) {
	char temp[4096]; stringbuilder sb(temp);
	if(!p)
		return;
	p->proc(sb);
	choose_manual(temp);
}