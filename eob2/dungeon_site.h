#pragma once

enum resid : unsigned short;

struct dungeon_site {
	resid type; // Resources of dungeon
	unsigned char habbits[2]; // Who dwelve here
	unsigned char key; // Key open all doors
	unsigned char special; // Special item find somewhere
	unsigned char language; // All messages in this language (by race)
	unsigned char level; // Dungeon level: 0 - is special surface, 1+ for underground.
};
