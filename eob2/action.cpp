#include "action.h"
#include "creature.h"
#include "class.h"
#include "race.h"

actioni* last_action;

static bool have_class(const flag32& classes, int type) {
	auto& ei = bsdata<classi>::elements[type];
	for(auto i = 0; i < 3; i++) {
		if(classes.is(ei.classes[i]))
			return true;
	}
	return false;
}

bool actioni::isallow(const creaturei* player) const {
	if(races && !races.is(player->race))
		return false;
	if(classes && !have_class(classes, player->character_class))
		return false;
	return true;
}
