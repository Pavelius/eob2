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

#include "array.h"
#include "io_stream.h"
#include "log.h"
#include "stringbuilder.h"

namespace {
struct translate {
	const char* id;
	const char* name;
};
}
static array source_name(sizeof(translate));

static int compare(const void* v1, const void* v2) {
	auto p1 = (const translate*)v1;
	auto p2 = (const translate*)v2;
	return szcmp(p1->id, p2->id);
}

static void update_elements() {
	if(!source_name)
		return;
	qsort(source_name.data, source_name.count, source_name.size(), compare);
}

static const char* read_string_v1(const char* p, stringbuilder& sb) {
	char sym; sb.clear();
	while(*p && *p != '\n' && *p != '\r') {
		if(p[0] == '\\' && p[1] == 'n') {
			sym = '\n';
			p += 2;
		} else {
			sym = *p;
			p++;
		}
		sb.addch(sym);
	}
	while(*p == '\n' || *p == '\r') {
		p = skipcr(p);
		p = skipsp(p);
	}
	return p;
}

static const char* read_string_v2(const char* p, stringbuilder& sb) {
	sb.clear();
	while(*p && *p != '#') {
		auto sym = *p++;
		switch(sym) {
		case '\n': case '\r':
			sym = '\n';
			p = skipspcr(p);
			break;
		}
		sb.addch(sym);
	}
	sb.trimr();
	return p;
}

static const char* read_identifier(const char* p, char* ps, const char* pe) {
	while(*p && (ischa(*p) || isnum(*p) || *p == '_' || *p == ' ')) {
		if(ps < pe)
			*ps++ = *p++;
		else
			break;
	}
	*ps = 0;
	return p;
}

static void apply_value(const char* id, const char* name) {
	id = szdup(id);
	name = szdup(name);
	if(source_name.findv(id, 0))
		return;
	auto p = (translate*)source_name.add();
	p->id = id;
	p->name = name;
}

static void readl_extend(const char* p, int& records_read) {
	char name[128], value[8192]; stringbuilder sb(value);
	while(*p && log::allowparse) {
		p = log::skipwscr(p);
		if(p[0] == '#')
			p = read_identifier(p + 1, name, name + sizeof(name) - 1);
		p = log::skipwscr(p);
		p = read_string_v2(p, sb);
		apply_value(name, value);
		records_read++;
	}
}

static void read_names(const char* url) {
	auto p = log::read(url, true);
	if(!p)
		return;
	char name[128], value[8192]; stringbuilder sb(value);
	auto records_read = 0;
	p = log::skipwscr(p);
	if(p[0] == '#')
		readl_extend(p, records_read);
	else {
		while(*p) {
			p = read_identifier(p, name, name + sizeof(name) - 1);
			if(p[0] != ':')
				break;
			p = skipsp(p + 1);
			p = read_string_v1(p, sb);
			apply_value(name, value);
			records_read++;
		}
	}
	log::close();
	update_elements();
}

static void save_names(const char* url) {
	io::file file(url, StreamText | StreamWrite);
	if(!file)
		return;
	auto records_write = 0;
	for(auto& e : source_name.records<translate>()) {
		if(e.name)
			continue;
		file << e.id << ": ";
		if(e.name) {
			file << e.name;
			records_write++;
		}
		file << "\r\n";
	}
}

#ifdef _DEBUG
static void deinitialize() {
	char temp[260]; stringbuilder sb(temp);
	sb.add("locale/%1/UnknownNames.txt", current_locale);
	save_names(temp);
}
#endif

static void check_translation() {
	log::context.url = "UnknownNames.txt";
	for(auto& e : source_name.records<translate>()) {
		if(e.name && e.name[0])
			continue;
		log::errorp(0, " Define translation for `%1`", e.id);
	}
}

void initialize_translation() {
	char temp[260]; stringbuilder sb(temp);
	sb.add("locale/%1", current_locale);
	log::readf(read_names, temp, "*.txt");
#ifdef _DEBUG
	check_translation();
	if(!log::errors)
		atexit(deinitialize);
#endif
}

const char* getnme(const char* id) {
	if(!id || id[0] == 0)
		return 0;
	if(!ischa(id[0]))
		return id;
	translate key = {id, 0};
	auto p = (translate*)bsearch(&key, source_name.data, source_name.getcount(), source_name.size(), compare);
	return p ? p->name : 0;
}

const char* getnme(const char* id, const char* p1) {
	char temp[128]; stringbuilder sb(temp);
	sb.addv(id, 0);
	sb.addv(p1, 0);
	return getnme(temp);
}

const char* getnm(const char* id) {
	if(!id || id[0] == 0)
		return "";
	if(!ischa(id[0]))
		return id;
	translate key = {id, 0};
	auto p = (translate*)bsearch(&key, source_name.data, source_name.getcount(), source_name.size(), compare);
	if(!p) {
#ifdef _DEBUG
		p = (translate*)source_name.add();
		p->id = szdup(id);
		p->name = 0;
		update_elements();
#endif
		return id;
	}
	if(!p->name || !p->name[0])
		return id;
	return p->name;
}

void check_description(const char* id, const char** psuffix) {
	if(!getnme(id))
		log::errorp(0, " Define translation for `%1`", id);
	for(auto p = psuffix; *p; p++) {
		auto name = ids(id, *p);
		if(!getnme(name))
			log::errorp(0, " Define translation for `%1`", name);
	}
}