#include "creature.h"
#include "unit.h"
#include "slice.h"

uniti party;

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

void join_party() {
	for(auto& e : party.units) {
		if(e)
			continue;
		e = player;
		break;
	}
}