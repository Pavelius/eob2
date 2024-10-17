#include "archive.h"
#include "creature.h"
#include "dungeon.h"
#include "party.h"
#include "variant.h"

static bool serial_game(const char* url, bool writemode) {
	io::file file(url, writemode ? StreamWrite : StreamRead);
	if(!file)
		return false;
	archive e(file, writemode);
	if(!e.signature("SAV"))
		return false;
	auto digital_signature = 0;
	auto digital_index = 25;
	digital_signature += bsreq_signature();
	digital_signature += bsreq_name_count_signature();
	digital_signature += sizeof(partyi) * digital_index;
	if(!e.checksum(digital_signature))
		return false;
	for(auto i = 0; i < 6; i++)
		e.set(characters[i]);
	e.set(party);
	e.set(loc);
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