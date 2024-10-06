#include "class.h"
#include "creature.h"
#include "location.h"
#include "modifier.h"
#include "party.h"
#include "race.h"
#include "script.h"

racei* last_race;
classi* last_class;

template<> void ftscript<racei>(int value, int bonus) {
	last_race = bsdata<racei>::elements + value;
}

template<> void ftscript<classi>(int value, int bonus) {
	last_class = bsdata<classi>::elements + value;
}

template<> void ftscript<feati>(int value, int bonus) {
	switch(modifier) {
	case Permanent:
		player->basic.setfeat((featn)value);
		break;
	default:
		player->setfeat((featn)value);
		break;
	}
}

template<> void ftscript<locationi>(int value, int bonus) {
	last_location = bsdata<locationi>::elements + value;
}

template<> void ftscript<abilityi>(int value, int bonus) {
	switch(modifier) {
	case Permanent:
		add_value(player->basic.abilities[value], bonus);
		break;
	default:
		add_value(player->abilities[value], bonus);
		break;
	}
}

template<> void ftscript<itemi>(int value, int bonus) {
	item v; v.create(value);
	player->additem(v);
}

static void attack_modify(int bonus) {
	ftscript<abilityi>(AttackMelee, bonus);
	ftscript<abilityi>(AttackRange, bonus);
}

static void enter_location(int bonus) {
	party.location = getbsi(last_location);
}

BSDATA(script) = {
	{"Attack", attack_modify},
	{"EnterLocation", enter_location},
};
BSDATAF(script)