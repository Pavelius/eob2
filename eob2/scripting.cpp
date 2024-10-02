#include "creature.h"
#include "modifier.h"
#include "script.h"

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

static void attack_modify(int bonus) {
	ftscript<abilityi>(AttackMelee, bonus);
	ftscript<abilityi>(AttackRange, bonus);
}

BSDATA(script) = {
	{"Attack", attack_modify},
};
BSDATAF(script)