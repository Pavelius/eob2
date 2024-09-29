#pragma once

#ifdef _MSC_VER
#define xva_start(v) (((const char*)&v) + sizeof(v))
#else
#define xva_start(v) (((const char*)&v) + sizeof(v)*4)
#endif

class stringbuilder {
	struct grammar;
	struct genderi;
	char*			p;
	char*			pb;
	const char*		pe;
	const char*		readformat(const char* format, const char* format_param);
	const char*		readvariable(const char* format);
	void			add(const char* s, const grammar* source, const char* def = 0);
public:
	typedef void (*fncustom)(stringbuilder& sb, const char* id);
	constexpr stringbuilder(char* pb, const char* pe) : p(pb), pb(pb), pe(pe) {}
	template<unsigned N> constexpr stringbuilder(char(&result)[N]) : stringbuilder(result, result + N - 1) {}
	constexpr operator char*() const { return pb; }
	explicit constexpr operator bool() const { return pb[0]; }
	static fncustom	custom;
	void			add(const char* format, ...) { addv(format, xva_start(format)); }
	void			add(char sym);
	void			addby(const char* s);
	void			addch(char sym);
	void			addint(int value, int precision, const int radix);
	void			adjective(const char* name, int m);
	void			addn(const char* format, ...) { addx('\n', format, xva_start(format)); }
	void			addnounf(const char* s);
	void			addnouni(const char* s);
	void			addnounx(const char* s);
	void			addnz(const char* format, unsigned count);
	void			addof(const char* s);
	void			adds(const char* format, ...) { addx(' ', format, xva_start(format)); }
	void			addsep(char separator);
	void			addsz() { if(p < pe) *p++ = 0; }
	void			addto(const char* s);
	void			addv(const char* format, const char* format_param);
	void			addx(char separator, const char* format, const char* format_param);
	void			addx(const char* separator, const char* format, const char* format_param);
	void			adduint(unsigned value, int precision, const int radix);
	const char*		begin() const { return pb; }
	void			change(char s1, char s2);
	void			change(const char* s1, const char* s2);
	void			clear() { pb[0] = 0; p = pb; }
	void			copy(const char* v);
	const char*		end() const { return pe; }
	char*			get() const { return p; }
	static int		getgender(const char* s);
	unsigned		getmaximum() const { return pe - pb - 1; }
	bool			isempthy() const { return !pb || pb[0] == 0; }
	bool			isfull() const { return p >= pe; }
	bool			ispos(const char* v) const { return p == v; }
	void			lower();
	const char*		psidf(const char* pb);
	const char*		psparam(const char* p);
	const char*		psstr(const char* p, char end_symbol);
	const char*		psstrlf(const char* p);
	void			set(char* v) { p = v; p[0] = 0; }
	size_t			size() const { return p - pb; }
	void			trimr();
	void			upper();
};
typedef const char* (*fntext)(const void* object, stringbuilder& sb);
typedef void (*fnstatus)(const void* object, stringbuilder& sb);
typedef void (*fnprint)(stringbuilder& sb);
typedef void (*fnoutput)(const char* format);

unsigned char lower_symbol(unsigned char sym);
unsigned char upper_symbol(unsigned char sym);

const char* getnm(const char* id);
const char*	getnme(const char* id);
const char* getnme(const char* id, const char* p1);

const char* ids(const char* p1, const char* p2);
const char* ids(const char* p1, const char* p2, const char* p3);
const char* psbon(const char* p, int& result);
const char*	psidf(const char* p, stringbuilder& result);
const char*	psnum(const char* p, long& result);
const char*	psnum(const char* p, int& result);
const char*	psnum(const char* p, short& result);
const char* str(const char* format, ...);
const char* str_count(const char* id, int count);
const char* skipcr(const char* format);
const char* skipsp(const char* p);
const char* skipspcr(const char* p);
const char* skipline(const char* p);
const char* szfind(const char* text, const char* value);
const char* szdup(const char* text);
const char* szdupz(const char* text);

int get_number(const char* p);
int get_line_number(const char* start, const char* position);
int	szcmpi(const char* p1, const char* p2);
int szcmp(const char* p1, const char* p2);

void default_string(stringbuilder& sb, const char* id);
bool equal(const char* s1, const char* s2);
void initialize_translation();
bool szstart(const char* text, const char* value);
bool szmatch(const char* text, const char* value);
bool szpmatch(const char* text, const char* pattern);

constexpr bool ischa(unsigned char s) { return (s >= 'A' && s <= 'Z') || (s >= 'a' && s <= 'z') || s >= 0xC0; }
constexpr bool isnum(unsigned char s) { return s >= '0' && s <= '9'; }
constexpr const char* zchr(const char* p, char e) { while(*p) { if(*p == e) return p; p++; } return 0; }
constexpr const char* zend(const char* p) { while(*p) p++; return p; }
constexpr size_t zlen(const char* p) { return zend(p) - p; }

template<class T> void ftinfo(const void* object, stringbuilder& sb);

extern char current_locale[4];