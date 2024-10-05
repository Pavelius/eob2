#include "bsdata.h"
#include "item.h"
#include "wearable.h"

static_assert(sizeof(item) == 4, "Size of item structure must be 4 bytes");

item* last_item;

void item::clear() {
	memset(this, 0, sizeof(*this));
}

const itemi& item::geti() const {
	return bsdata<itemi>::elements[type];
}

bool item::isallow(wearn v) const {
	auto n = geti().wear;
	switch(v) {
	case LeftRing:
	case RightRing:
		return n == LeftRing
			|| n == RightRing;
	case RightHand:
		return n == LeftHand
			|| n == RightHand;
	default:
		if(v>=Backpack && v<=LastBackpack)
			return true;
		return n == v;
	}
}

void item::create(int value) {
	type = value;
	flags = 0;
	power = 0;
	count = 0;
}
