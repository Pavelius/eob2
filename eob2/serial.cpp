#include "archive.h"
#include "boost.h"
#include "creature.h"
#include "dungeon.h"
#include "party.h"
#include "quest.h"
#include "variant.h"

extern spella spells_prepared[6];

static bool check_game(archive& e) {
	unsigned long checksum_total = 0;
	int checksum_index = 1;
	checksum_total += SeeIllusionary * (checksum_index++);
	checksum_total += sizeof(partyi) * (checksum_index++);
	checksum_total += sizeof(spells_prepared) * (checksum_index++);
	checksum_total += sizeof(dungeoni) * (checksum_index++);
	checksum_total += sizeof(boosti) * (checksum_index++);
	checksum_total += sizeof(spellseta) * (checksum_index++);
	return e.checksum(checksum_total);
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
