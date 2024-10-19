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

creaturei* characters[6];
partyi party;

int get_party_index(const creaturei* target) {
	for(auto i = 0; i < 6; i++) {
		if(characters[i] == target)
			return i;
	}
	return -1;
}

static void update_party_side() {
	for(auto i = 0; i < lenghtof(characters); i++) {
		if(characters[i])
			characters[i]->side = i;
	}
}

void join_party(int bonus) {
	for(auto& e : characters) {
		if(e)
			continue;
		e = player;
		break;
	}
	update_party_side();
}

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
	for(auto p : characters) {
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
	for(auto p : characters) {
		if(p && !p->isdisabled())
			p->addexp(value);
	}
}