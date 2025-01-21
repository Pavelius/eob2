#include "archive.h"
#include "boost.h"
#include "creature.h"
#include "dungeon.h"
#include "party.h"
#include "quest.h"
#include "shop.h"
#include "variant.h"

extern spella spells_prepared[6];

// Special case character serialization
template<> void archive::set(creaturei*& value) {
}

static bool check_game(archive& e) {
	unsigned long total = 0;
	int index = 1;
	total += ImmuneIllusion * (index++);
	total += sizeof(partyi) * (index++);
	total += sizeof(spells_prepared) * (index++);
	total += sizeof(dungeoni) * (index++);
	total += sizeof(boosti) * (index++);
	total += sizeof(spellseta) * (index++);
	total += bsdata<shopi>::source.getcount() * (index++);
	return e.checksum(total);
}

static bool serial_game(const char* url, bool writemode) {
	io::file file(url, writemode ? StreamWrite : StreamRead);
	if(!file)
		return false;
	archive e(file, writemode);
	if(!e.signature("SAV"))
		return false;
	if(!check_game(e))
		return false;
	for(auto i = 0; i < 6; i++)
		e.setpointer((void**)&characters[i], bsdata<creaturei>::source);
	e.set(party);
	e.set(spells_prepared, sizeof(spells_prepared));
	e.set(bsdata<spellseta>::elements, sizeof(spellseta) * bsdata<spellseta>::source.getmaximum());
	e.set(loc);
	e.set(bsdata<boosti>::source); // This one can be problem if change metadata. Look boost.h.
	e.set(bsdata<creaturei>::source);
	e.set(bsdata<dungeoni>::source);
	for(auto& v : bsdata<shopi>())
		e.set(v.items);
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

void delete_game(const char* id) {
	io::file::remove(get_save(id));
}
