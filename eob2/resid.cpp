#include "bsdata.h"
#include "io_stream.h"
#include "resid.h"

static const char* root = "art/core";
static const char* root_dungeon = "art/dungeons";
static const char* root_monsters = "art/monsters";

BSDATA(residi) = {
	{"NONE"},

	{"FONT6", root},
	{"FONT8", root},
	{"border", root},
	{"outtake", root},
	{"decors", root},
	{"chargen", root},
	{"chargenb", root},
	{"compass", root},
	{"invent", root},
	{"items", root},
	{"itemgs", root},
	{"itemgl", root},
	{"menu", root},
	{"playfld", root},
	{"intro", root},
	{"portm", root},
	{"thrown", root},
	{"xspl", root},
	{"npc", root},
	{"bplace", root},
	{"adventure", root},
	{"buildngs", root},
	{"dungeons", root},
	{"crystal", root},
	{"scenes", root},

	{"blue", root_dungeon},
	{"brick", root_dungeon},
	{"crimson", root_dungeon},
	{"drow", root_dungeon},
	{"dung", root_dungeon},
	{"green", root_dungeon},
	{"forest", root_dungeon},
	{"mezz", root_dungeon},
	{"silver", root_dungeon},
	{"xanatha", root_dungeon},

	{"ANKHEG", root_monsters},
	{"ANT", root_monsters},
	{"BLDRAGON", root_monsters},
	{"BUGBEAR", root_monsters},
	{"CLERIC1", root_monsters},
	{"CLERIC2", root_monsters},
	{"CLERIC3", root_monsters},
	{"DRAGON", root_monsters},
	{"DWARF", root_monsters},
	{"FLIND", root_monsters},
	{"GHOUL", root_monsters},
	{"GOBLIN", root_monsters},
	{"GUARD1", root_monsters},
	{"GUARD2", root_monsters},
	{"KOBOLD", root_monsters},
	{"KUOTOA", root_monsters},
	{"LEECH", root_monsters},
	{"ORC", root_monsters},
	{"SHADOW", root_monsters},
	{"SKELETON", root_monsters},
	{"SKELWAR", root_monsters},
	{"SPIDER1", root_monsters},
	{"WIGHT", root_monsters},
	{"WOLF", root_monsters},
	{"ZOMBIE", root_monsters},
};
assert_enum(residi, ZOMBIE)

sprite* residi::get() {
	if(data)
		return data;
	if(!data && !error) {
		char temp[260];
		szurl(temp, folder, id, "pma");
		data = (sprite*)loadb(temp);
		if(!data) {
			szurl(temp, folder, id, "FNT");
			data = (sprite*)loadb(temp);
		}
		error = (data != 0);
	}
	return data;
}

sprite* gres(resid i) {
	if(i == NONE)
		return 0;
	return bsdata<residi>::elements[i].get();
}

sprite* gres(const char* id) {
	auto p = bsdata<residi>::find(id);
	if(!p)
		return 0;
	return p->get();
}

void clear_sprites() {
	for(auto& e : bsdata<residi>()) {
		if(e.data) {
			delete[]((char*)e.data);
			e.data = 0;
		}
	}
}
