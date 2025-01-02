#include "randomizer.h"
#include "shop.h"

void shopi::clear() {
	memset(items, 0, sizeof(items));
}

item* shopi::add() {
	for(auto& e : items) {
		if(!e)
			return &e;
	}
	return 0;
}

void shopi::additem(int magic_bonus, int chance_magic, int chance_cursed, bool identified) {
	itemi* pi = random_variant(effect);
	if(!pi)
		return;
	last_item = add();
	last_item->create(pi);
	last_item->createpower(magic_bonus, chance_magic, chance_cursed);
	if(identified)
		last_item->identify(1);
}