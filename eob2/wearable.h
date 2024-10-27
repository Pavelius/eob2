#pragma once

#include "item.h"

enum wearn : unsigned char {
	Backpack, Edible, Drinkable, Readable, Usable, LastBackpack = Backpack + 13,
	Head, Neck, Body, RightHand, LeftHand, RightRing, LeftRing, Elbow, Legs, Quiver,
	FirstBelt, SecondBelt, LastBelt,
	FirstInvertory = Backpack, LastInvertory = LastBelt
};
struct weari : nameable{
};

struct wearable {
	item		wears[LastBelt+1];
	void		additem(item& v);
	slice<item> backpack() { return slice<item>(wears + Backpack, wears + LastBackpack); }
	bool		consume(const item* pi);
	void		equip(item& v);
	slice<item> equipment() { return slice<item>(wears + Head, wears + Legs); }
};
