#pragma once

#include "item.h"

struct shopi : nameable {
	item		items[8];
	variants	effect; // Refresh items
	void		clear();
	item*		add();
	void		additem(int magic_bonus, int chance_magic, int chance_cursed, bool identified);
};
