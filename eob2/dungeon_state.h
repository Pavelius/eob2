#pragma once

#include "flagable.h"
#include "posable.h"
#include "wallmessage.h"

struct dungeon_state {
	posable			up, down; // where is stairs located
	posable			portal; // where is portal
	posable			special; // where is special item dropped
	posable			lair; // where is lair located
	posable			feature; // where is dungeon feature locatied (if any)
	short unsigned	messages; // count of messages
	short unsigned	secrets_found; // count of secret rooms found (used secret button)
	short unsigned	elements; // count of corridors
	short unsigned	bones; // count of bones
	short unsigned	gems; // count of gems
	short unsigned	relicts; // count of books and holy symbols originally placed
	short unsigned	items; // total count of items originally placed
	short unsigned	items_lying; // total count of items laying on ground
	short unsigned	overlays; // total count of overlays
	short unsigned	monsters; // total count of monsters
	short unsigned	monsters_alive; // total alive monsters
	short unsigned	monsters_killed; // total killed monsters
	short unsigned	wallmessages[MessageHabbits]; // count of variable messages
	flagable<2>		goals; // Reaching goals by party
	bool			boss_alive;
	void clear();
};
