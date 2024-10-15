#pragma once

#include "posable.h"
#include "flagable.h"

struct dungeon_state {
	posable			up, down; // where is stairs located
	posable			portal; // where is portal
	posable			special; // where is special item dropped
	posable			lair; // where is lair located
	posable			feature; // where is dungeon feature locatied (if any)
	short unsigned	messages; // count of messages
	short unsigned	secrets; // count of secret rooms
	short unsigned	secrets_found; // count of secret rooms found (used secret button)
	short unsigned	artifacts; // count of artifact items originally placed
	short unsigned	rings; // count of magical rings
	short unsigned	weapons; // count of magical weapons
	short unsigned	elements; // count of corridors
	short unsigned	traps; // count of traps
	short unsigned	bones; // count of bones
	short unsigned	gems; // count of gems
	short unsigned	relicts; // count of books and holy symbols originally placed
	short unsigned	items; // total count of items originally placed
	short unsigned	overlays; // total count of overlays
	short unsigned	monsters; // total count of monsters
	short unsigned	monsters_alive; // total alive monsters
	short unsigned	monsters_killed; // total killed monsters
	flagable<2>		goals; // Reaching goals by party
	bool			boss_alive;
	void clear();
};
