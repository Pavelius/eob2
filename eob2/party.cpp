#include "bsdata.h"
#include "creature.h"
#include "party.h"

BSDATA(partystati) = {
	{"GoldPiece"},
	{"Reputation"},
	{"Blessing"},
	{"StartYear"},
	{"Minutes"},
};
partyi party;

void add_party(partystatn id, int value) {
	party.abilities[id] += value;
}

int getparty(partystatn id) {
	return party.abilities[id];
}

void join_party() {
	for(auto& e : party.units) {
		if(e)
			continue;
		e = player;
		break;
	}
}

bool is_dead_line() {
	return party.abilities[StopDeadLine] != 0
		&& party.abilities[StopDeadLine] > party.abilities[Minutes];
}

void skip_hours(int value) {
	add_party(Minutes, 60 * value);
}