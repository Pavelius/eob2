#include "bsdata.h"
#include "item.h"

static_assert(sizeof(item) == 4, "Size of item structure must be 4 bytes");

item* last_item;

const itemi& item::geti() const {
	return bsdata<itemi>::elements[type];
}