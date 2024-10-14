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

bool parse_abilities(stringbuilder& sb, const char* id) {
	auto p = bsdata<partystati>::find(id);
	if(!p)
		return false;
	auto index = p - bsdata<partystati>::elements;
	sb.add("%1i", party.abilities[index]);
	return true;
}

void set_party_position(pointc v) {
	party.x = v.x;
	party.y = v.y;
}

void set_party_position(pointc v, directions d) {
	set_party_position(v);
	party.d = d;
}

int party_count() {
	int result = 0;
	for(auto p : party.units) {
		if(p && !p->isdisabled())
			result++;
	}
	return result;
}

void party_addexp(int value) {
	auto n = party_count();
	if(!n)
		return;
	value = (value + n - 1) / n;
	for(auto p : party.units) {
		if(p && !p->isdisabled())
			p->addexp(value);
	}
}