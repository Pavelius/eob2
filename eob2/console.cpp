#include "console.h"
#include "stringbuilder.h"
#include "slice.h"

char console_text[512];

static void delete_line() {
	auto p = zchr(console_text, '\n');
	if(p) {
		p++;
		memcpy(console_text, p, zlen(p) + 1);
	} else
		console_text[0] = 0;
}

static int get_line_count() {
	if(console_text[0] == 0)
		return 0;
	auto count = 1;
	auto p = console_text;
	while(*p) {
		if(*p == '\n')
			count++;
		p++;
	}
	return count;
}

void consolenl() {
	if(console_text[0])
		console("\n");
}

void console(const char* format, ...) {
	consolev(format, xva_start(format));
}

void consolev(const char* format, const char* format_param) {
	if(!format || !format[0])
		return;
	if(get_line_count() > 3)
		delete_line();
	auto m = zlen(console_text);
	stringbuilder sb(console_text);
	sb.set(console_text + m);
	sb.addv(format, format_param);
}

void clear_console() {
	console_text[0] = 0;
}