#pragma once

#include "reference.h"

struct boosti {
	unsigned	stamp;
	referencei	target;
	short		type, param;
	explicit operator bool() const { return stamp != 0; }
};
typedef void(*fnclearboost)(referencei target, short type, short param);

void add_boost(unsigned stamp, referencei target, short type, short param);
void clear_boost(unsigned current_stamp, fnclearboost clearing);

boosti* find_boost(referencei target, short type, short param);