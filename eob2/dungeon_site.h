#pragma once

enum resid : unsigned short;
enum racen : unsigned char;

struct dungeon_site {
	resid type; // Resources of dungeon
	racen language; // All messages in this language (by race)
	unsigned char level; // Dungeon level: 0 - is special surface, 1+ for underground.
	unsigned char habbits[2]; // Who dwelve here
	unsigned char boss, minions; // Boss with minions can be present on level lair
	unsigned char key; // Key open all doors
	unsigned char special; // Special item find somewhere
	unsigned char webs, barrels, eggs, graves; // Count of special corridor features in dungeon
	unsigned char trap; // Type of all dungeon traps
	char cursed; // Chance to all items found be cursed
	char magical; // Chance to all items found be magical
	constexpr explicit operator bool() const { return type != (resid)0; }
};