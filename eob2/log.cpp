#include "io_stream.h"
#include "log.h"
#include "slice.h"
#include "stringbuilder.h"

bool log::allowparse = true;
int log::errors;
log::contexti log::context;

static void print_file_error(const char* format) {
	static io::file file("errors.txt", StreamWrite | StreamText);
	if(!file)
		return;
	file << format;
}
fnoutput log::print_proc = print_file_error;

void log::contexti::clear() {
	memset(this, 0, sizeof(*this));
}

void log::printv(const char* format) {
	if(!print_proc)
		return;
	print_proc(format);
}

void log::printv(const char* format, const char* format_param) {
	char temp[4096]; stringbuilder sb(temp);
	sb.addv(format, format_param);
	printv(temp);
}

void log::print(const char* format, ...) {
   XVA_FORMAT(format);
	printv(format, format_param);
}

void log::println() {
	printv("\r\n");
}

const char* log::read(const char* url, bool error_if_not_exist) {
	context.clear();
	auto p_alloc = loadt(url);
	if(!p_alloc) {
		if(error_if_not_exist)
			errorp(0, "Can't find file '%1'", url);
		return 0;
	}
	context.header = "Error in file `%1`:";
	context.url = url;
	context.file = p_alloc;
	return p_alloc;
}

void log::close() {
	if(context.file)
		delete context.file;
	context.file = 0;
}

void log::errorv(const char* position, const char* format, const char* format_param) {
	errors++;
	if(context.header) {
		print(context.header, context.url);
		println();
		context.header = 0;
	}
	if(position && context.file)
		print(" Line %1i: ", get_line_number(context.file, position));
	printv(format, format_param);
	println();
}

void log::errorp(const char* position, const char* format, ...) {
   XVA_FORMAT(format);
	errorv(position, format, format_param);
}

const char* log::skipws(const char* p) {
	while(true) {
		if(*p == ' ' || *p == 9) {
			p++;
			continue;
		}
		if(p[0] == '\\' && (p[1] == 10 || p[1] == 13)) { // Trail symbol
			p++;
			while(*p == 10 || *p == 13)
				p++;
			continue;
		}
		if(p[0] == '/' && p[1] == '/') { // End line comment
			p += 2;
			while(*p && *p != 10 && *p != 13)
				p++;
		}
		if(p[0] == '/' && p[1] == '*') { // Complex comment
			p += 2;
			while(*p && !(p[0] == '*' && p[1] == '/'))
				p++;
		}
		break;
	}
	return p;
}

const char* log::skipwscr(const char* p) {
	while(true) {
		p = skipws(p);
		if(*p == 10 || *p == 13) {
			while(*p == 10 || *p == 13)
				p++;
			continue;
		}
		break;
	}
	return p;
}

void log::readf(fnread proc, const char* folder, const char* filter) {
	for(io::file::find find(folder); find; find.next()) {
		auto pn = find.name();
		if(!pn || pn[0] == '.')
			continue;
		if(filter) {
			if(!szpmatch(pn, filter))
				continue;
		}
		char temp2[260];
		find.fullname(temp2);
		proc(temp2);
	}
}

void log::readlf(fnread proc, const char* folder, const char* filter) {
	char temp[260]; stringbuilder sb(temp);
	sb.add("%1/%2", folder, current_locale);
	readf(proc, temp, filter);
}

static const char* example(const char* p, stringbuilder& sb) {
	while(*p && *p != '\n' && *p != '\r') {
		if(sb.isfull())
			break;
		sb.add(*p++);
	}
	return sb.begin();
}

bool log::checksym(const char* p, char sym) {
	if(!allowparse)
		return false;
	if(sym == '\n') {
		if(*p != '\n' && *p != '\r') {
			log::errorp(p, "Expected symbol line feed");
			allowparse = false;
			return false;
		}
	} else if(*p != sym) {
		char result[] = {sym, 0};
		char string[16]; stringbuilder sb(string); sb.clear();
		log::errorp(p, "Expected symbol `%1`, but you have string `%2`", result, example(p, sb));
		allowparse = false;
		return false;
	}
	return true;
}

bool log::errorpresent() {
	if(errors > 0)
		return true;
	if(io::file::exist("errors.txt"))
		io::file::remove("errors.txt");
	return false;
}
