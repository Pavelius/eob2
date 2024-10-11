#include "archive.h"
#include "creature.h"
#include "dungeon.h"
#include "party.h"
#include "variant.h"

//static void add(archive& a, void* data, unsigned size, short unsigned& count) {
//	a.set(count);
//	a.set(data, size * count);
//}
//
//template<class T> static void add(archive& a, T* data, short unsigned& count) {
//	add(a, data, sizeof(T), count);
//}
//
//template<> void archive::set<dungeoni>(dungeoni& ev) {
//	set(ev.type);
//	set(ev.level);
//	set(ev.language);
//	set(ev.key);
//	set(ev.wand);
//	set(ev.habbits, sizeof(ev.habbits));
//	set(ev.state);
//	add(*this, ev.items, ev.state.items);
//}

//template<> void archive::set<creaturei*>(creaturei*& value) {
//	unsigned char ref[2];
//	if(!writemode) {
//		set(ref);
//		if(ref[0] == 0)
//			value = bsdata<creaturei>::elements + ref[1];
//	} else {
//		if(bsdata<creaturei>::have(value)) {
//			ref[0] = 0;
//			ref[1] = value - bsdata<creaturei>::elements;
//			set(ref);
//		}
//	}
//}

template<> void archive::set<partyi>(partyi& ev) {
	for(auto i = 0; i < 6; i++)
		set(ev.units[i]);
	set(ev.location);
	set(ev.abilities, sizeof(ev.abilities));
}

static bool serial_game(const char* url, bool writemode) {
	io::file file(url, writemode ? StreamWrite : StreamRead);
	if(!file)
		return false;
	archive e(file, writemode);
	if(!e.signature("SAV"))
		return false;
	if(!e.version(0, 1))
		return false;
	e.set(party);
	e.set(bsdata<creaturei>::source);
	e.set(bsdata<dungeoni>::source);
	return true;
}

static const char* get_file_name(const char* id, const char* folder, const char* ext) {
	static char temp[260]; stringbuilder sb(temp);
	sb.add("%1/%2.%3", folder, id, ext);
	return temp;
}

static const char* get_save(const char* id) {
	return get_file_name(id, "saves", "sav");
}

void save_game(const char* id) {
	serial_game(get_save(id), true);
}

bool read_game(const char* id) {
	if(!serial_game(get_save(id), false))
		return false;
	continue_game();
	return true;
}