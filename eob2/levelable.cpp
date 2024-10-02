#include "bsdata.h"
#include "class.h"
#include "levelable.h"

int levelable::getclassindex(int id) const {
	auto& e = bsdata<classi>::elements[character_class];
	for(char i = 0; i < e.count; i++) {
		if(e.classes[i] == id)
			return i;
	}
	return -1;
}

int levelable::getlevel(int v) const {
	auto& e = bsdata<classi>::elements[character_class];
	if(e.classes[0] == v)
		return levels[0];
	else if(e.classes[1] == v)
		return levels[1];
	else if(e.classes[2] == v)
		return levels[2];
	return 0;
}
