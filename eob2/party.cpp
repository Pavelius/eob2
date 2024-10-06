#include "bsdata.h"
#include "creature.h"
#include "party.h"

BSDATA(partystati) = {
	{"GoldPiece"},
	{"Reputation"},
	{"Blessing"},
	{"StartYear"},
	{"StartDeadLine"},
	{"StopDeadLine"},
	{"Minutes"},
};
assert_enum(partystati, Minutes)
partyi party;

void add_party(partystatn id, int value) {
	party.abilities[id] += value;
}

int getparty(partystatn id) {
	return party.abilities[id];
}

bool is_dead_line() {
	return party.abilities[StopDeadLine] != 0
		&& party.abilities[StopDeadLine] > party.abilities[Minutes];
}