#include "action.h"
#include "creature.h"
#include "class.h"
#include "race.h"
#include "pushvalue.h"
#include "script.h"

actioni* last_action;

bool have_class(const flag32& classes, classn type) {
	auto& ei = bsdata<classi>::elements[type];
	for(auto i = 0; i < ei.count; i++) {
		if(classes.is(ei.classes[i]))
			return true;
	}
	return false;
}

bool party_have_class(flag32 source) {
	for(auto p : characters) {
		if(!p || p->isdisabled())
			continue;
		if(have_class(source, p->character_class))
			return true;
	}
	return false;
}

bool party_have_race(flag32 source) {
	for(auto p : characters) {
		if(!p || p->isdisabled())
			continue;
		if(source.is(p->race))
			return true;
	}
	return false;
}

bool party_have_alignment(flag32 source) {
	for(auto p : characters) {
		if(!p || p->isdisabled())
			continue;
		if(source.is(p->alignment))
			return true;
	}
	return false;
}

bool allow_item(creaturei* player, const variants& filter) {
	pushvalue push(last_item);
	for(auto& e : player->wears) {
		if(!e)
			continue;
		last_item = &e;
		if(script_allow(filter))
			return true;
	}
	return false;
}

bool allow_item(const variants& filter) {
	for(auto p : characters) {
		if(!p || !p->isready())
			continue;
		if(allow_item(p, filter))
			return true;
	}
	return false;
}