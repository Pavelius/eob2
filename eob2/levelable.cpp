#include "bsdata.h"
#include "class.h"
#include "levelable.h"

int levelable::getlevel(classn v) const {
	auto& e = bsdata<classi>::elements[character_class];
	if(e.classes.data[0] == v)
		return levels[0];
	else if(e.classes.data[0] == v)
		return levels[1];
	else if(e.classes.data[0] == v)
		return levels[2];
	return 0;
}
