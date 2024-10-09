#include "answers.h"
#include "bsdata.h"
#include "draw.h"
#include "picture.h"
#include "resid.h"
#include "slice.h"
#include "view.h"

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

static void parse_special(const char* p) {
	char temp[128]; stringbuilder sb(temp);
	p = skipsp(sb.psidf(p + 1));
	auto pn = bsdata<residi>::find(temp);
	if(pn) {
		picture.id = (resid)(pn - bsdata<residi>::elements);
		picture.frame = getparam(p);
	}
}

void* dialogv(const char* cancel, const char* format) {
	char temp[512]; stringbuilder sb(temp);
	void* result = 0;
	while(format[0]) {
		sb.clear(); format = sb.psstrlf(format);
		format = skipspcr(format);
		if(temp[0] == '/') {
			parse_special(temp);
			continue;
		}
		if(format[0])
			result = show_message(temp, false, getnm("Continue"), KeyEnter);
		else {
			auto cancel_key = KeyEscape;
			if(!an) {
				cancel = getnm("Continue");
				cancel_key = KeyEnter;
			}
			result = show_message(temp, true, cancel, cancel_key);
		}
	}
	return result;
}

void* dialogv(const char* cancel, const char* format, const char* format_param) {
	char temp[4096]; stringbuilder sb(temp);
	sb.addv(format, format_param);
	return dialogv(cancel, temp);
}

void* dialog(const char* cancel, const char* format, ...) {
	return dialogv(cancel, format, xva_start(format));
}