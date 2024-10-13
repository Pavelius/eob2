#include "dungeon_state.h"
#include "slice.h"

void dungeon_state::clear() {
	memset(this, 0, sizeof(*this));
	up.clear();
	down.clear();
	portal.clear();
	special.clear();
	lair.clear();
	feature.clear();
}