#include "boost.h"
#include "bsdata.h"
#include "cell.h"
#include "class.h"
#include "console.h"
#include "creature.h"
#include "direction.h"
#include "draw.h"
#include "dungeon.h"
#include "math.h"
#include "party.h"
#include "rand.h"
#include "view.h"

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
creaturei* monsters[6];
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

static int get_monster_best(pointc v, abilityn a) {
	auto result = 0;
	for(auto& e : loc->monsters) {
		if(e != v)
			continue;
		auto n = e.get(a);
		if(result < n)
			result = n;
	}
	return result;
}

void surprise_roll(creaturei** creatures, int bonus) {
	for(auto i = 0; i < 6; i++) {
		auto p = creatures[i];
		if(p && !p->roll(Alertness, bonus)) {
			consolen(getnm("SurpriseFailed"), p->getname());
			p->set(Surprised);
		}
	}
}

static void monster_move(pointc v, directions d) {
	auto n = to(v, d);
	if(n == party) {
		set_monster_moved(v);
		// CHEAT: Monster have best sneak. Party only median.
		turnto(party, to(d, Down), true, get_monster_best(v, Sneaky));
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

static void check_poison() {
	if(!player->is(PoisonLevel))
		return;
	auto penalty = player->get(PoisonLevel) / 5;
	if(!player->roll(SaveVsPoison, -penalty))
		player->damage(Poison, 1, 100);
	player->add(PoisonLevel, -1);
}

static void update_every_turn() {
	check_poison();
}

static bool all_party_disabled() {
	for(auto p : characters) {
		if(p && !p->isdisabled())
			return false;
	}
	return true;
}

void all_party(fnevent proc, bool skip_disabled) {
	auto push_player = player;
	for(auto p : characters) {
		if(!p)
			continue;
		if(skip_disabled && p->isdisabled())
			continue;
		player = p; proc();
	}
	player = push_player;
}

static void update_floor_buttons() {
	unsigned char map[mpy][mpx] = {0};
	if(party)
		map[party.y][party.x]++;
	for(auto& e : loc->monsters) {
		if(!e)
			continue;
		if(map[e.y][e.x] > 0)
			continue;
		map[e.y][e.x]++;
	}
	for(auto& e : loc->items) {
		if(!e)
			continue;
		if(map[e.y][e.x] > 0)
			continue;
		map[e.y][e.x]++;
	}
	pointc pt;
	for(pt.y = 0; pt.y < mpy; pt.y++) {
		for(pt.x = 0; pt.x < mpx; pt.x++) {
			auto t = loc->get(pt);
			if(t == CellButton) {
				auto new_active = map[pt.y][pt.x] > 0;
				auto active = loc->is(pt, CellActive);
				if(active != new_active && new_active) {
					auto po = loc->getlinked(pt);
					if(po) {
						switch(po->type) {
						case CellTrapLauncher:
							break;
						}
					}
				}
				if(new_active)
					loc->set(pt, CellActive);
				else
					loc->remove(pt, CellActive);
			}
		}
	}
}

void all_creatures(fnevent proc) {
	all_party(proc, true);
	if(!loc)
		return;
	auto push_player = player;
	for(auto& e : loc->monsters) {
		if(!e)
			continue;
		player = &e; proc();
	}
	player = push_player;
}

void pass_round() {
	clear_boost(party.abilities[Minutes]);
	monsters_movement();
	add_party(Minutes, 1);
	update_floor_buttons();
	all_creatures(update_every_round);
	if((party.abilities[Minutes] % 6) == 0)
		all_creatures(update_every_turn);
	fix_animate();
	if(all_party_disabled()) {
		message_box(getnm("AllPartyDead"));
		set_next_scene(main_menu);
	}
}

void pass_hours(int value) {
	add_party(Minutes, 60 * value);
	clear_boost(party.abilities[Minutes]);
	all_creatures(update_every_round);
	for(auto i = 0; i < 6 * value; i++)
		all_creatures(update_every_turn);
}

int party_median(creaturei** creatures, abilityn v) {
	auto count = 0;
	auto value = 0;
	for(int i = 0; i < 6; i++) {
		if(!creatures[i])
			continue;
		value += creatures[i]->abilities[v];
		count++;
	}
	if(!count)
		return 0;
	return value / count;
}

quest* partyi::getquest() const {
	return getbs<quest>(quest_id);
}

locationi* partyi::getlocation() const {
	return getbs<locationi>(location_id);
}