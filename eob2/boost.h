#pragma once

#include "reference.h"
#include "variant.h"

struct boosti {
	unsigned	stamp;
	referencei	target;
	variant		effect;
	explicit operator bool() const { return stamp != 0; }
};

void add_boost(unsigned stamp, referencei target, variant effect);
void clear_boost(unsigned current_stamp);

boosti* find_boost(unsigned short location, unsigned short creature, variant effect);