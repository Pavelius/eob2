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
	{"overlays", root},

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

	{"ankheg", root_monsters},
	{"ant", root_monsters},
	{"bldragon", root_monsters},
	{"bugbear", root_monsters},
	{"cleric1", root_monsters},
	{"cleric2", root_monsters},
	{"cleric3", root_monsters},
	{"dragon", root_monsters},
	{"dwarf", root_monsters},
	{"flind", root_monsters},
	{"ghoul", root_monsters},
	{"goblin", root_monsters},
	{"guard1", root_monsters},
	{"guard2", root_monsters},
	{"kobold", root_monsters},
	{"kuotoa", root_monsters},
	{"leech", root_monsters},
	{"orc", root_monsters},
	{"shadow", root_monsters},
	{"skeleton", root_monsters},
	{"skelwar", root_monsters},
	{"spider1", root_monsters},
	{"wight", root_monsters},
	{"wolf", root_monsters},
	{"zombie", root_monsters},
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
			data = (sprite*)loadb(temp, 0);
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
