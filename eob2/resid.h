#pragma once

struct sprite;

enum resid : unsigned short {
	NONE,
	FONT6, FONT8,
	BORDER, OUTTAKE, DECORS,
	CHARGEN, CHARGENB, COMPASS, INVENT, ITEMS, ITEMGS, ITEMGL,
	MENU, PLAYFLD, INTRO, PORTM, THROWN, XSPL,
	NPC, BPLACE, ADVENTURE, BUILDNGS, DUNGEONS, CRYSTAL, SCENES, OVERLAYS,
	BLUE, BRICK, CRIMSON, DROW, DUNG, GREEN, FOREST, MEZZ, SILVER, XANATHA,
	// Monsters
	ANKHEG, ANT, BLDRAGON, BUGBEAR, CLERIC1, CLERIC2, CLERIC3, DRAGON, DWARF, FLIND,
	GHOUL, GOBLIN, GUARD1, GUARD2, KOBOLD, KUOTOA, LEECH, ORC,
	SHADOW, SKELETON, SKELWAR, SPIDER1, WIGHT, WOLF, ZOMBIE,
};
struct residi {
	const char*		id;
	const char*		folder;
	sprite*			data;
	bool			error;
	sprite*			get();
};
void clear_sprites();
sprite* gres(resid i);
sprite* gres(const char* id);