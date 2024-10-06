#include "adat.h"
#include "stringbuilder.h"

#pragma once

typedef void(*fnanswer)(int index, const void* value, const char* text, unsigned key, fnevent press_event);

struct answers {
	struct element {
		const void* value;
		const char* text;
		unsigned	key;
	};
	char buffer[2048];
	stringbuilder sc;
	adat<element, 32> elements;
	answers() : sc(buffer) {}
	constexpr operator bool() const { return elements.count != 0; }
	void			add(const void* value, const char* name, ...) { addv(value, name, xva_start(name), 0); }
	void			addv(const void* value, const char* name, const char* format, unsigned key);
	const element*	begin() const { return elements.data; }
	element*		begin() { return elements.data; }
	void			clear();
	static int		compare(const void* v1, const void* v2);
	const element*	end() const { return elements.end(); }
	bool			have(const void* pv) const { return elements.have(pv); }
	int				getcount() const { return elements.getcount(); }
	const char*		getname(void* v);
	int				indexof(const void* v) const { return elements.indexof(v); }
	void*			random() const;
	void			remove(int index) { elements.remove(index, 1); }
	void			sort();
};
extern answers an;
struct pushanswer {
	answers			answer;
	pushanswer() : answer(an) { an.clear(); }
	~pushanswer() { an = answer; }
};
extern bool show_interactive;