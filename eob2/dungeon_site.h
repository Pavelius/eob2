#pragma once

enum resid : unsigned short;
enum racen : unsigned char;

struct dungeon_site {
	resid type; // Resources of dungeon
	unsigned char level; // Dungeon level: 0 - is special surface, 1+ for underground.
	unsigned char habbits[2]; // Who dwelve here
	unsigned char key; // Key open all doors
	unsigned char special; // Special item find somewhere
	racen language; // All messages in this language (by race)
	char cursed; // Chance to all items found be cursed
	char magical; // Chance to all items found be magical
};
