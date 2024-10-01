#include "unit.h"
#include "slice.h"

void uniti::clear() {
	memset(this, 0, sizeof(*this));
}

bool uniti::is(const creaturei* v) const {
	for(auto p : units) {
		if(p == v)
			return true;
	}
	return false;
}