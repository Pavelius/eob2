#include "bsdata.h"
#include "draw.h"
#include "picture.h"
#include "resid.h"
#include "slice.h"
#include "timer.h"

//////////////////////////////
// RICH COMMAND FORMAT EXAMPLE
//////////////////////////////

using namespace draw;

static const char* text_start_string;
static int text_start_horiz;
static bool force_full_render;
static point maxcaret;
unsigned draw::text_flags;

static bool equaln(const char*& p, const char* name) {
	int n = zlen(name);
	if(memcmp(p, name, n) != 0)
		return false;
	switch(p[n]) {
	case '\n': case ' ': case '\t':
		p = skipsp(p + n);
		return true;
	default:
		return false;
	}
}

static const char* getparam(const char*& p, stringbuilder& sb) {
	auto pb = sb.get();
	if(p[0] == '\'')
		p = sb.psstr(p + 1, '\'');
	else if(p[0] == '\"')
		p = sb.psstr(p + 1, '\"');
	else if(ischa(*p))
		p = sb.psidf(p);
	sb.addsz();
	p = skipsp(p);
	return pb;
}

static int getparam(const char*& p) {
	if(isnum(p[0]) || (p[0] == '-' && isnum(p[1]))) {
		int result = 0;
		p = psnum(p, result);
		p = skipsp(p);
		return result;
	}
	return 0;
}

static const char* skip_line_nlf(const char* p) {
	while(*p && p[0] != 10 && p[0] != 13) p++;
	return p;
}

static const char* skip_line(const char* p) {
	return skipcr(skip_line_nlf(p));
}

static const char* paint_text(const char* p) {
	auto p1 = skip_line_nlf(p);
	text(p, p1 - p, text_flags);
	return p1;
}

static const char* parse_widget_command(const char* p, int x1, int x2) {
	auto push_caret = caret;
	auto push_width = width;
	auto push_height = height;
	char temp[1024]; stringbuilder sb(temp);
	height = 0;
	while(*p && *p != 10 && *p != 13) {
		if(equaln(p, "right")) {
			text_flags &= ~AlignMask;
			text_flags |= AlignRight;
			continue;
		} else if(equaln(p, "left")) {
			text_flags &= ~AlignMask;
			text_flags |= AlignLeft;
			continue;
		} else if(equaln(p, "center")) {
			text_flags &= ~AlignMask;
			text_flags |= AlignCenter;
			continue;
		} else if(equaln(p, "bold")) {
			text_flags |= TextBold;
			continue;
		} else if(equaln(p, "x")) {
			caret.x += getparam(p);
			continue;
		} else if(equaln(p, "y")) {
			caret.y += getparam(p);
			continue;
		} else if(equaln(p, "w")) {
			width = getparam(p);
			continue;
		} else if(equaln(p, "h")) {
			height = getparam(p);
			continue;
		} else if(equaln(p, "text"))
			p = paint_text(p);
		else {
			sb.clear();
			p = skipsp(sb.psidf(p));
			auto pn = bsdata<residi>::find(temp);
			if(pn) {
				picture.id = (resid)(pn - bsdata<residi>::elements);
				picture.frame = getparam(p);
			}
			continue;
		}
		break;
	}
	p = skip_line(p);
	if(caret.x + width > maxcaret.x)
		maxcaret.x = caret.x + width;
	if(caret.y + height > maxcaret.y)
		maxcaret.y = caret.y + height;
	caret = push_caret;
	width = push_width;
	caret.y += height;
	if(caret.y > maxcaret.y)
		maxcaret.y = caret.y;
	height = push_height;
	return p;
}

static bool match(const char** string, const char* name) {
	int n = zlen(name);
	if(memcmp(*string, name, n) != 0)
		return false;
	(*string) += n;
	return true;
}

static int textfbc(const char* string, int width) {
	if(!font)
		return 0;
	auto c = -1;
	auto w = 0;
	const char* s1 = string;
	while(true) {
		unsigned s = *s1++;
		if(s == 0x20 || s == 9)
			c = s1 - string;
		else if(s == 0 || s == 10 || s == 13) {
			c = s1 - string - 1;
			break;
		}
		w += textw(s);
		if(w > width)
			break;
	}
	if(c < -1)
		c = 0;
	return c;
}

static const char* textfn(const char* p, int x1, int x2, int y2, color fore) {
	auto push_fore = draw::fore;
	auto push_caret = caret;
	draw::fore = fore;
	auto dy = texth();
	auto width = x2 - x1;
	while(*p && *p != 10 && *p != 13) {
		auto c = textfbc(p, width);
		if(!c)
			break;
		auto w = textw(p, c);
		caret.x = aligned(x1, width, text_flags, w);
		text(p, c, text_flags);
		caret.y += dy;
		p = skipsp(p + c);
		if(caret.y > y2)
			break;
	}
	if(*p == 10 || *p == 13) {
		p = skipcr(p);
		if(caret.y == push_caret.y)
			caret.y += dy;
	}
	caret.x = push_caret.x;
	draw::fore = push_fore;
	return p;
}

void draw::textf(const char* p) {
	auto push_flags = text_flags;
	auto push_width = width;
	auto push_height = height;
	auto push_font = font;
	maxcaret.clear();
	text_start_string = 0;
	text_start_horiz = 0;
	int x1 = caret.x, y1 = caret.y, x2 = caret.x + width, y2 = caret.y + 0xFFFF;
	while(p[0]) {
		if(caret.y < clipping.y1) {
			text_start_string = p;
			text_start_horiz = caret.y - clipping.y1;
		} else if(!force_full_render && caret.y > clipping.y2)
			break;
		if(match(&p, "#")) // Header 3
			p = textfn(skipsp(p), x1, x2, y2, colors::yellow);
		else if(p[0] == '/' && ischa(p[1]))
			p = parse_widget_command(p + 1, x1, x2);
		else
			p = textfn(p, x1, x2, y2, fore);
		if(maxcaret.y < caret.y)
			maxcaret.y = caret.y;
	}
	maxcaret.x -= x1; maxcaret.y -= y1;
	text_flags = push_flags;
	width = push_width;
	height = push_height;
}

void draw::textfs(const char* string) {
	auto push_caret = caret;
	auto push_clipping = clipping;
	auto push_maxcaret = maxcaret;
	auto push_force = force_full_render;
	force_full_render = true;
	clipping.clear(); caret = {};
	textf(string);
	force_full_render = push_force;
	clipping = push_clipping;
	caret = push_caret;
	width = maxcaret.x;
	height = maxcaret.y;
	maxcaret = push_maxcaret;
}