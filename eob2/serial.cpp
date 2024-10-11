#include "archive.h"
#include "creature.h"
#include "dungeon.h"
#include "party.h"
#include "variant.h"

template<> void archive::set<creaturei*>(creaturei*& value) {
	unsigned char ref[2];
	if(!writemode) {
		set(ref);
		if(ref[0] == 0)
			value = bsdata<creaturei>::elements + ref[1];
	} else {
		if(bsdata<creaturei>::have(value)) {
			ref[0] = 0;
			ref[1] = value - bsdata<creaturei>::elements;
			set(ref);
		}
	}
}

template<> void archive::set<partyi>(partyi & value) {
	for(auto i = 0; i < 6; i++)
		set(value.units[i]);
	set(value.location);
	set(value.abilities, sizeof(value.abilities));
}

static void serial_game(const char* url, bool writemode) {
	io::file file(url, writemode ? StreamWrite : StreamRead);
	if(!file)
		return;
	archive e(file, writemode);
	e.set(party);
	e.set(bsdata<creaturei>::source);
	e.set(bsdata<dungeoni>::source);
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