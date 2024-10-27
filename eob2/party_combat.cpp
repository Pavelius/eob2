#include "adat.h"
#include "creature.h"
#include "direction.h"
#include "dungeon.h"
#include "party.h"
#include "rand.h"
#include "view.h"

static adat<creaturei*, 16> combatants;
int enemy_distance;

static size_t shrink_creatures(creaturei** dest, creaturei** units, size_t count) {
	auto ps = dest;
	auto pb = units;
	auto pe = units + count;
	while(pb < pe) {
		if(*pb)
			*ps++ = *pb;
		pb++;
	}
	return ps - dest;
}

static int compare_creatures(const void* v1, const void* v2) {
	return (*((creaturei**)v1))->initiative - (*((creaturei**)v2))->initiative;
}

static bool select_combatants(pointc position) {
	loc->getmonsters(combatants.data, position);
	combatants.count = shrink_creatures(combatants.data, combatants.data, 6);
	if(!combatants)
		return false;
	combatants.count += shrink_creatures(combatants.data + combatants.count, characters, 6);
	// Lowest initiative win, so positive speed is substracted
	for(auto p : combatants) {
		if(p->is(Surprised))
			p->initiative = 50; // Move last
		else
			p->initiative = xrand(1, 10) - p->get(Speed);
	}
	qsort(combatants.data, combatants.count, sizeof(combatants.data[0]), compare_creatures);
	return true;
}

static bool select_combatants(pointc v, directions d) {
	enemy_distance = 0;
	for(auto i = 0; i < 3; i++) {
		v = to(v, d);
		if(!v)
			return false;
		if(select_combatants(v)) {
			enemy_distance = i + 1;
			turnto(v, to(d, Down));
			animation_update();
			return true;
		}
	}
	return false;
}

static void select_combatants(creaturei** result, bool enemies) {
	memset(result, 0, sizeof(result[0]) * 6);
	for(auto p : combatants) {
		if(p->isdisabled())
			continue;
		auto ismonster = p->ismonster();
		if(ismonster != enemies)
			continue;
		if(ismonster) {
			auto side = get_side(p->side, party.d);
			if(!result[side])
				result[side] = p;
		} else {
			if(!result[p->side])
				result[p->side] = p;
		}
	}
}

static creaturei* get_opponent(bool left, bool enemies) {
	creaturei* result[6]; select_combatants(result, enemies);
	for(auto y = 0; y < 3; y++) {
		auto n = left ? 0 : 1;
		if(d100() < 30) // Randomly select nearest
			n = n ? 0 : 1;
		auto p = result[y * 2 + n];
		if(p)
			return p;
		p = result[y * 2 + (n + 1) % 2];
		if(p)
			return p;
	}
	return 0;
}

static void single_main_attack(creaturei* player, wearn wear, creaturei* enemy, int bonus, int multiplier) {
	auto number_attacks = 2;
	if(player->is(WeaponSpecialist) && player->isspecialist(&player->wears[wear].geti())) {
		bonus += 1;
		number_attacks += 1;
	}
	if(party.abilities[Minutes] % 2)
		number_attacks += 1;
	number_attacks /= 2;
	while(number_attacks-- > 0)
		player->attack(enemy, wear, bonus, multiplier);
}

static void make_full_attack(creaturei* player, creaturei* enemy, int bonus, int multiplier) {
	if(!enemy)
		return;
	fix_monster_attack(player);
	auto wp1 = player->wears[RightHand];
	auto wp2 = player->wears[LeftHand];
	auto wp3 = player->wears[Head];
	if(wp1.is(TwoHanded) || !wp2.isweapon())
		wp2.clear();
	if(!wp3.isweapon())
		wp3.clear();
	if(wp2) {
		single_main_attack(player, RightHand, enemy, bonus + player->gethitpenalty(-4), multiplier);
		player->attack(enemy, LeftHand, bonus + player->gethitpenalty(-6), multiplier);
	} else
		single_main_attack(player, RightHand, enemy, bonus, multiplier);
	if(wp3)
		player->attack(enemy, Head, bonus, multiplier);
	fix_monster_attack_end(player);
}

void turnto(pointc v, directions d, bool test_surprise, int sneaky_bonus) {
	if(!d)
		return;
	if(v == party) {
		if(test_surprise) {
			if(party.d != d)
				surprise_roll(characters, sneaky_bonus);
		}
		set_party_position(v, d);
	} else {
		creaturei* result[6] = {}; loc->getmonsters(result, v, party.d);
		for(int i = 0; i < 6; i++) {
			auto pc = result[i];
			if(!pc)
				continue;
			if(test_surprise) {
				if(pc->d != d) {
					surprise_roll(result, sneaky_bonus);
					test_surprise = false;
				}
			}
			pc->d = d;
		}
	}
}

void make_melee_attacks() {
	auto v = to(party, party.d);
	auto d = to(party.d, Down);
	turnto(v, d);
	animation_update();
	enemy_distance = 0;
	if(!select_combatants(v))
		return;
	enemy_distance = 1;
	for(auto p : combatants) {
		if(p->isdisabled())
			continue;
		// RULE: Surprised creatures do not move first round in combat
		if(p->is(Surprised)) {
			p->remove(Surprised);
			continue;
		}
		if(p->ismonster()) {
			auto left_side = (get_side(p->side, d) % 2) == 0;
			if(p->is(Large))
				left_side = (rand() % 2);
			make_full_attack(p, get_opponent(left_side, false), 0, 1);
		} else {
			auto left_side = (p->side % 2) == 0;
			make_full_attack(p, get_opponent(left_side, true), 0, 1);
		}
		animation_update();
		fix_animate();
		p->set(Moved);
	}
}

void make_attacks() {
	if(!select_combatants(party, party.d))
		return;
	auto d = to(party.d, Down);
	for(auto p : combatants) {
		if(p->isdisabled())
			continue;
		// RULE: Surprised creatures do not move first round in combat
		if(p->is(Surprised)) {
			p->remove(Surprised);
			continue;
		}
		// If we can only shoot
		if(enemy_distance > 1 && !p->wears[RightHand].isranged())
			continue;
		if(p->ismonster()) {
			auto left_side = (get_side(p->side, d) % 2) == 0;
			if(p->is(Large))
				left_side = (rand() % 2);
			make_full_attack(p, get_opponent(left_side, false), 0, 1);
		} else {
			auto left_side = (p->side % 2) == 0;
			make_full_attack(p, get_opponent(left_side, true), 0, 1);
		}
		fix_animate();
		p->set(Moved);
	}
}