#include "draw.h"
#include "slice.h"
#include "stringbuilder.h"

///////////////////////////////////////////////////////////
// RICH COMMAND FORMAT EXAMPLE
//
// /x 64 image Bitmap 0 'art/images'
// /lf w 200 h 64
// /x 10 w 64 h 64 center text Some text centered in rectangle.

using namespace draw;

int draw::tab_pixels = 0;
unsigned draw::text_flags;

static const char* text_start_string;
static int text_start_horiz;
static point maxcaret;

static void apply_line_feed(int x1, int dy) {
	if(maxcaret.x < caret.x)
		maxcaret.x = caret.x;
	caret.x = x1;
	caret.y += dy;
	if(maxcaret.y < caret.y)
		maxcaret.y = caret.y;
}

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

static const char* read_special_string(const char* p, stringbuilder& sb) {
	while(*p) {
		if(p[0] == '-' && p[1] == '!' && p[2] == '\'')
			p = read_special_string(p + 3, sb);
		else if(p[0] == '\'' && p[1] == '!' && p[2] == '-') {
			p = skipsp(p + 3);
			break;
		}
	}
	return p;
}

static const char* getparam(const char*& p, stringbuilder& sb) {
	auto pb = sb.get();
	if(p[0] == '-' && p[1] == '!' && p[2] == '\'')
		p = read_special_string(p + 3, sb);
	else if(p[0] == '\'')
		p = sb.psstr(p + 1, '\'');
	else if(p[0] == '\"')
		p = sb.psstr(p + 1, '\"');
	else if(ischa(*p))
		p = sb.psidf(p);
	sb.addsz();
	p = skipsp(p);
	return pb;
}

static const char* read_lf(const char* p, stringbuilder& sb) {
	auto pb = sb.get();
	while(*p && !(*p == 10 || *p == 13))
		sb.addch(*p++);
	sb.addsz();
	return skipsp(p);
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

static const char* skip_line(const char* p) {
	for(; *p && p[0] != 10 && p[0] != 13; p++);
	return skipcr(p);
}

static bool match(const char** string, const char* name) {
	int n = zlen(name);
	if(memcmp(*string, name, n) != 0)
		return false;
	(*string) += n;
	return true;
}

static int gettabwidth() {
	return tab_pixels ? tab_pixels : textw(' ') * 4;
}

static const char* textspc(const char* p, int x1) {
	int tb;
	while(true) {
		switch(p[0]) {
		case ' ':
			p++;
			caret.x += textw(' ');
			continue;
		case '\t':
			p++;
			tb = gettabwidth();
			caret.x = x1 + ((caret.x - x1 + tb) / tb) * tb;
			continue;
		}
		break;
	}
	return p;
}

static const char* wholeline(const char* p) {
	while(*p && *p != 10 && *p != 13)
		p++;
	return p;
}

static const char* word(const char* text) {
	if(text[0] == ':')
		return text + 1;
	while(((unsigned char)*text) > 0x20 && *text != '*' && *text != '[' && *text != ']' && *text != ':')
		text++;
	return text;
}

static const char* citate(const char* p, int x1, int x2, color new_fore, const sprite* new_font) {
	pushfore push_fore(new_fore);
	pushfont push_font(new_font);
	caret.x = x1;
	while(p[0]) {
		if(p[0] == '-' && p[1] == '#' && p[2] == '-' && (p[3] == 13 || p[3] == 10 || p[3] == 0)) {
			p = skipspcr(p + 3);
			break;
		}
		auto p2 = skipspcr(wholeline(p));
		text(p, p2 - p);
		p = p2;
		apply_line_feed(x1, texth());
	}
	apply_line_feed(caret.x, 0);
	return p;
}

static const char* textfln(const char* p, int x1, int x2, color new_fore, const sprite* new_font) {
	pushfore push_fore(new_fore);
	pushfont push_font(new_font);
	unsigned flags = text_flags;
	while(true) {
		if(p[0] == '*' && p[1] == '*') {
			p += 2;
			if(flags & TextBold)
				flags &= ~TextBold;
			else
				flags |= TextBold;
			continue;
		} else if(p[0] == '*') {
			p++;
			if(flags & TextItalic)
				flags &= ~TextItalic;
			else
				flags |= TextItalic;
			continue;
		} else if(p[0] == '[' && p[1] == '[')
			p++;
		else if(p[0] == ']' && p[1] == ']')
			p++;
		else if(p[0] == '[') {
			p++;
			switch(*p) {
			case '+':
				p++;
				fore = colors::green;
				break;
			case '-':
				p++;
				fore = colors::red;
				break;
			case '!':
				p++;
				fore = colors::active;
				break;
			case '#':
				p++;
				flags |= TextUscope;
				fore = colors::special;
				break;
			default:
				fore = colors::special;
				break;
			}
		} else if(p[0] == ']') {
			p++;
			flags &= ~TextUscope;
			fore = new_fore;
		}
		// ќбработаем пробелы и табул€цию
		p = textspc(p, x1);
		auto w = 0;
		if(p[0] == ':' && p[1] >= '0' && p[1] <= '9') {
			auto index = 0;
			p = psnum(p + 1, index);
			if(*p == ':')
				p++;
			if(metrics::icons) {
				w = metrics::icons->get(index).sx;
				image(caret.x, caret.y + (font->height + 1) / 2, metrics::icons, index, 0);
			}
		} else {
			const char* p2 = word(p);
			if(p2==p)
				break;
			w = textw(p, p2 - p);
			if(caret.x + w > x2)
				apply_line_feed(x1, texth());
			text(p, p2 - p, flags);
			p = p2;
		}
		caret.x += w;
		p = textspc(p, x1);
		if(p[0] == 0 || p[0] == 10 || p[0] == 13) {
			p = skipcr(p);
			apply_line_feed(x1, texth());
			break;
		}
	}
	apply_line_feed(caret.x, 0);
	return p;
}

static const char* parse_widget_command(const char* p) {
	auto next_line = false;
	auto push_caret = caret;
	auto push_width = width;
	auto push_height = height; height = texth();
	auto push_alpha = alpha;
	auto flags = text_flags;
	char temp[4096]; stringbuilder sb(temp);
	while(*p) {
		if(equaln(p, "ca")) {
			fore = colors::active;
			continue;
		} else if(equaln(p, "ct")) {
			fore = colors::text;
			continue;
		} else if(equaln(p, "red")) {
			fore = colors::red;
			continue;
		} else if(equaln(p, "green")) {
			fore = colors::green;
			continue;
		} else if(equaln(p, "blue")) {
			fore = colors::blue;
			continue;
		} else if(equaln(p, "f1")) {
			font = metrics::h1;
			continue;
		} else if(equaln(p, "fn")) {
			font = metrics::font;
			continue;
		} else if(equaln(p, "lf")) {
			next_line = true;
			continue;
		} else if(equaln(p, "h")) {
			height = getparam(p);
			continue;
		} else if(equaln(p, "w")) {
			width = getparam(p);
			continue;
		} else if(equaln(p, "x")) {
			caret.x = push_caret.x + getparam(p);
			continue;
		} else if(equaln(p, "y")) {
			caret.y = push_caret.y + getparam(p);
			continue;
		} else if(equaln(p, "a")) {
			alpha = getparam(p);
			continue;
		} else if(equaln(p, "right")) {
			flags = AlignRightCenter;
			continue;
		} else if(equaln(p, "center")) {
			flags = AlignCenterCenter;
			continue;
		} else if(equaln(p, "fill")) {
			rectf();
			continue;
		} else if(equaln(p, "fore")) {
			fore.r = (unsigned char)getparam(p);
			fore.g = (unsigned char)getparam(p);
			fore.b = (unsigned char)getparam(p);
			continue;
		} else if(equaln(p, "bold")) {
			flags |= TextBold | TextItalic;
			continue;
		} else if(equaln(p, "text")) {
			sb.clear();
			p = read_lf(p, sb);
			texta(temp, flags);
		} else if(equaln(p, "tab"))
			tab_pixels = getparam(p);
		p = skip_line(p);
		break;
	}
	if(caret.x + width > maxcaret.x)
		maxcaret.x = caret.x + width;
	if(caret.y + height > maxcaret.y)
		maxcaret.y = caret.y + height;
	caret = push_caret;
	width = push_width;
	text_flags = flags;
	alpha = push_alpha;
	if(next_line) {
		caret.y += height + metrics::padding;
		if(caret.y > maxcaret.y)
			maxcaret.y = caret.y;
	}
	height = push_height;
	return p;
}

static void separator(int x2) {
	auto push_x = caret.x;
	caret.y += 2;
	caret.x -= metrics::border;
	auto push_fore = fore;
	fore = colors::border;
	line(x2 + metrics::border - 1, caret.y);
	fore = push_fore;
	caret.x = push_x;
	caret.y += 2;
}

static const char* bullet_list(const char* p, int x2) {
	auto dx = texth() / 2;
	auto rd = texth() / 6;
	auto push_caret = caret;
	caret.x += dx + 2;
	caret.y += dx;
	circlef(rd);
	circle(rd);
	caret = push_caret;
	caret.x += texth();
	p = textfln(p, caret.x, x2, fore, font);
	caret.x = push_caret.x;
	return p;
}

void textf(const char* p) {
	maxcaret.clear();
	text_start_string = 0;
	text_start_horiz = 0;
	if(!p || p[0] == 0)
		return;
	pushfore push_fore;
	pushfont push_font;
	auto push_width = width;
	auto push_height = height;
	auto push_tab = tab_pixels;
	auto push_x = caret.x;
	int x1 = caret.x, y1 = caret.y, x2 = caret.x + width;
	while(p[0]) {
		if(caret.y < clipping.y1) {
			text_start_string = p;
			text_start_horiz = caret.y - clipping.y1;
		}
		auto p1 = p;
		if(p[0] == '/' && ischa(p[1]))
			p = parse_widget_command(p + 1);
		else
			p = textfln(p, caret.x, x2, fore, font);
		if(p1 == p)
			break;
	}
	maxcaret.x -= x1; maxcaret.y -= y1;
	if(!maxcaret.y) {
		maxcaret.y = texth();
		caret.y += texth();
	}
	tab_pixels = push_tab;
	caret.x = push_x;
	width = push_width;
	height = push_height;
}

void textf(const char* string, const char*& cashe_string, int& cashe_origin) {
	textf(string);
	cashe_string = text_start_string;
	cashe_origin = text_start_horiz;
}

void textfs(const char* string) {
	auto push_caret = caret;
	auto push_clipping = clipping;
	auto push_maxcaret = maxcaret;
	clipping.clear(); caret = {};
	textf(string);
	clipping = push_clipping;
	caret = push_caret;
	width = maxcaret.x;
	height = maxcaret.y;
	maxcaret = push_maxcaret;
}

void textf(const char* content, const char*& cash_string, int& cash_origin, int& current, int& maximum) {
	rectpush push;
	if(current < 0) {
		cash_string = 0;
		current = 0;
	}
	if(!maximum) {
		auto push_height = height;
		textfs(content);
		maximum = height;
		height = push_height;
	}
	if(current > maximum) {
		cash_string = 0;
		current = 0;
	}
	if(!cash_string)
		textf(content, cash_string, cash_origin);
	else {
		caret.y -= cash_origin;
		textf(cash_string);
	}
}
