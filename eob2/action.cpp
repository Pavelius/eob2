#include "action.h"
#include "creature.h"
#include "class.h"
#include "race.h"
#include "script.h"

actioni* last_action;

bool have_class(const flag32& classes, classn type) {
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
	if(restrict_classes && have_class(classes, player->character_class))
		return false;
	if(alignment && !alignment.is(player->alignment))
		return false;
	if(filter && !script_allow(filter))
		return false;
	return true;
}
