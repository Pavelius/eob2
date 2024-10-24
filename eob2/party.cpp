#include "boost.h"
#include "bsdata.h"
#include "class.h"
#include "console.h"
#include "creature.h"
#include "direction.h"
#include "dungeon.h"
#include "math.h"
#include "party.h"
#include "rand.h"

BSDATA(partystati) = {
	{"GoldPiece"},
	{"Reputation"},
	{"Blessing"},
	{"StartYear"},
	{"StartDeadLine"},
	{"StopDeadLine"},
	{"Minutes"},
	{"EffectCount"},
};
assert_enum(partystati, EffectCount)

creaturei* characters[6];
spella spells_prepared[6];
partyi party;
partystatn last_variable;

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

void party_addexp_per_killed(int hd) {
	for(auto p : characters) {
		if(!p || p->isdisabled())
			continue;
		auto pc = bsdata<classi>::elements + p->type;
		p->addexp(hd * pc->exp_per_hd / 100);
	}
}

char* get_spells_prepared(const creaturei* target) {
	auto i = get_party_index(target);
	if(i == -1)
		return 0;
	return spells_prepared[i];
}

spellseta* get_spells_known(const creaturei* target) {
	auto i = getbsi(target);
	if(i == -1)
		return 0;
	return bsdata<spellseta>::elements + i;
}

static void monsters_stop(pointc v) {
	if(!v || !loc)
		return;
	for(auto& e : loc->monsters) {
		if(e != v)
			continue;
		e.set(Moved);
	}
}

static bool can_see_party(pointc v, directions d) {
	for(auto i = 0; i < 3; i++) {
		v = to(v, d);
		if(!v || !loc->ispassable(v))
			return false;
		if(v == party)
			return true;
	}
	return false;
}

static void set_monster_moved(pointc v) {
	for(auto& e : loc->monsters) {
		if(e != v)
			continue;
		e.set(Moved);
	}
}

void test_surprise(pointc v) {
	if(party == v) {
		consolen(getnm("AmbushTest"));
	} else{

	}
}

static void monster_move(pointc v, directions d) {
	auto n = to(v, d);
	if(n == party) {
		set_monster_moved(v);
		auto can_surprise = false;
		turnto(party, to(d, Down), &can_surprise);
		if(can_surprise)
			test_surprise(party);
		monster_interaction();
		return;
	}
	if(!n || loc->ismonster(n) || !loc->ispassable(n))
		return;
	for(auto& e : loc->monsters) {
		if(e != v)
			continue;
		e.d = d;
		e.x = n.x;
		e.y = n.y;
		e.set(Moved);
	}
}

static directions random_free_look(pointc v, directions d) {
	directions source[] = {Up, Left, Right, Down};
	if(d100() < 50)
		iswap(source[1], source[2]);
	for(auto nd : source) {
		auto d1 = to(d, nd);
		auto v1 = to(v, d1);
		if(v1 && loc->ispassable(v1))
			return d1;
	}
	return Center;
}

static void monsters_movement() {
	if(!loc)
		return;
	for(auto& e : loc->monsters) {
		if(!e || e.isdisabled() || e.is(Moved))
			continue;
		if(can_see_party(e, e.d))
			monster_move(e, e.d);
		else if(d100() < 45) {
			auto d = random_free_look(e, e.d);
			if(d != Center)
				monster_move(e, d);
		} else
			monsters_stop(e);
	}
}

static void update_every_round() {
	player->remove(Moved);
	update_player();
}

static void update_every_turn() {
}

static void allcreatures(fnevent proc) {
	auto push_player = player;
	for(auto p : characters) {
		if(!p || p->isdisabled())
			continue;
		player = p; proc();
	}
	if(loc) {
		for(auto& e : loc->monsters) {
			if(!e)
				continue;
			player = &e; proc();
		}
	}
	player = push_player;
}

void pass_round() {
	clear_boost(party.abilities[Minutes]);
	monsters_movement();
	add_party(Minutes, 1);
	allcreatures(update_every_round);
}

void pass_hours(int value) {
	add_party(Minutes, 60 * value);
	clear_boost(party.abilities[Minutes]);
	allcreatures(update_every_round);
	for(auto i = 0; i < 6 * value; i++)
		allcreatures(update_every_turn);
}
