#pragma once

#include "adat.h"
#include "flagable.h"
#include "goal.h"
#include "posable.h"
#include "wallmessage.h"

struct dungeon_state {
	posable			up, down; // where is stairs located
	posable			portal; // where is portal
	posable			special; // where is special item dropped
	adat<posable, 8> features; // where is dungeon features locatied (if any)
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
	short unsigned	traps_disabled; // total disabled traps
	short unsigned	locks_open; // total opened locks by theif tools or by key
	short unsigned	total_passable; // total cell passable (include buttons and pits)
	short unsigned	explored_passable; // total cell passable (include buttons and pits) explored
	short unsigned	wallmessages[MessageHabbits]; // count of variable messages
	goalf			goals; // Reaching goals by party
	void clear();
};
