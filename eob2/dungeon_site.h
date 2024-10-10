#pragma once

enum resid : unsigned char;

struct dungeon_site {
	resid type; // Resources of dungeon
	unsigned char habbits[2]; // Who dwelve here
	unsigned char key; // Key open all doors
	unsigned char wand; // Special find somewhere
	unsigned char language; // All messages in this language
};
