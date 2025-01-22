/////////////////////////////////////////////////////////////////////////
// 
// Copyright 2024 Pavel Chistyakov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http ://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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
	void			add(const void* value, const char* name, ...) { XVA_FORMAT(name); addv(value, name, format_param, 0); }
	void			addv(const void* value, const char* name, const char* format, unsigned key);
	const element*	begin() const { return elements.data; }
	element*		begin() { return elements.data; }
	void			clear();
	static int		compare(const void* v1, const void* v2);
	const element*	end() const { return elements.end(); }
	size_t			findvalue(const void* pv) const;
	bool			have(const void* pv) const { return elements.have(pv); }
	size_t			getcount() const { return elements.getcount(); }
	const char*		getname(void* v);
	int				indexof(const void* v) const { return elements.indexof(v); }
	void*			random() const;
	void			remove(int index) { elements.remove(index, 1); }
	void			sort();
};
extern answers an;
struct pushanswer {
	answers	answer;
	pushanswer() : answer(an) { an.clear(); }
	~pushanswer() { an = answer; }
};
extern bool show_interactive;
