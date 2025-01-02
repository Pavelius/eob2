#pragma once

#include "item.h"

struct shopi : nameable {
	item		items[8];
	unsigned	stamp;
	char		days;
	dice		count;
	variants	effect; // Refresh items
	void		clear();
	item*		add();
	void		additem(int magic_bonus, int chance_magic, int chance_cursed, bool identified);
	size_t		getsize() const;
	void		update(unsigned new_stamp);
};
extern shopi* last_shop;