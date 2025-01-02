#include "rand.h"
#include "randomizer.h"
#include "shop.h"

shopi* last_shop;

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

size_t shopi::getsize() const {
	auto n = 0;
	for(auto& e : items) {
		if(e)
			n++;
	}
	return n;
}

void shopi::update(unsigned new_stamp) {
	if(stamp >= new_stamp)
		return;
	stamp = new_stamp;
	stamp += xrand(days * 24 * 60, days * 2 * 24 * 60);
	clear();
	auto new_count = count.roll();
	for(auto i = 0; i < new_count; i++)
		additem(5, 100, 0, true);
}