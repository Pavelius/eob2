#include "archive.h"
#include "boost.h"
#include "creature.h"
#include "dungeon.h"
#include "party.h"
#include "quest.h"
#include "shop.h"
#include "variant.h"

extern spella spells_prepared[6];

BSARH(quest) = {
	BSRAW(stage),
	BSRAW(history),
	BSRAW(dungeon),
	{}};
BSARH(shopi) = {
	BSRAW(items),
	{}};

template<> void archive::set<quest>(quest*& value) {
	setpointerbyname((void**)&value, bsdata<quest>::source);
}

void clear_game(int bonus) {
	loc = last_dungeon = 0;
	last_quest = 0;
	party.clear();
	last_exit.clear();
	memset(spells_prepared, 0, sizeof(spells_prepared));
	memset(bsdata<spellseta>::elements, 0, bsdata<spellseta>::source.getmaximum() * sizeof(bsdata<spellseta>::elements[0]));
	bsdata<boosti>::source.clear();
	bsdata<creaturei>::source.clear();
	bsdata<dungeoni>::source.clear();
	archive::cleanup<quest>();
	archive::cleanup<shopi>();
	create_game_quests();
}

static bool check_game_content(archive& e) {
	unsigned long total = 0;
	int index = 1;
	total += sizeof(partyi) * (index++);
	total += sizeof(creaturei) * (index++);
	total += sizeof(dungeoni) * (index++);
	total += sizeof(boosti) * (index++);
	total += sizeof(spellseta) * (index++);
	total += bsdata<spellseta>::source.getmaximum() * (index++);
	total += ImmuneIllusion * (index++);
	total += sizeof(spells_prepared) * (index++);
	total += bsdata<itemi>::source.getcount() * (index++); // Because in dungeon and shop struct we have `item` object with requisit `type`.
	total += bsdata<monsteri>::source.getcount() * (index++); // Because in creature struct we have monster_id.
	return e.checksum(total);
}

static bool serial_game(const char* url, bool writemode) {
	io::file file(url, writemode ? StreamWrite : StreamRead);
	if(!file)
		return false;
	archive e(file, writemode);
	if(!e.signature("SAV"))
		return false;
	if(!check_game_content(e))
		return false;
	for(auto i = 0; i < 6; i++)
		e.set(characters[i]);
	e.set(loc);
	e.set(last_dungeon);
	e.set(last_exit);
	e.set(last_quest);
	e.set(party);
	e.set(spells_prepared, sizeof(spells_prepared));
	e.set(bsdata<spellseta>::elements, sizeof(spellseta) * bsdata<spellseta>::source.getmaximum());
	e.set(bsdata<boosti>::source);
	e.set(bsdata<creaturei>::source);
	e.set(bsdata<dungeoni>::source);
	e.setpartial<shopi>();
	e.setpartial<quest>();
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
