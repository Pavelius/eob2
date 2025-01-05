#include "archive.h"
#include "boost.h"
#include "creature.h"
#include "dungeon.h"
#include "party.h"
#include "quest.h"
#include "variant.h"

extern spella spells_prepared[6];

static unsigned long strhash(const char* id) {
	if(!id)
		return 0;
	unsigned long total = 0;
	for(auto i = 0; id[i]; i++)
		total += ((unsigned char)id[i]) * (i + 1);
	return total;
}

static unsigned long check_metadata() {
	unsigned long total = 0;
	int index = 1;
	for(auto& e : bsdata<varianti>())
		total += strhash(e.id) * (index++);
	return total;
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
	total += bsdata<varianti>::source.getcount() * (index++);
	total += check_metadata() * (index++);
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
		e.set(characters[i]);
	e.set(party);
	e.set(spells_prepared, sizeof(spells_prepared));
	e.set(bsdata<spellseta>::elements, sizeof(spellseta) * bsdata<spellseta>::source.getmaximum());
	e.set(loc);
	e.set(bsdata<boosti>::source);
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

void delete_game(const char* id) {
	io::file::remove(get_save(id));
}
