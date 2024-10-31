#include "adat.h"
#include "console.h"
#include "creature.h"
#include "direction.h"
#include "dungeon.h"
#include "party.h"
#include "math.h"
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
			p->initiative = 100; // Move last
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

static void single_attack(creaturei* defender, wearn slot, int bonus, int multiplier) {
	auto& weapon = player->wears[slot];
	auto power = weapon.getpower();
	auto chance_critical = 20;
	auto attack_damage = player->getdamage(bonus, slot, defender->is(Large));
	auto damage_type = weapon.geti().damage_type;
	auto isrange = weapon.isranged();
	auto ammo = weapon.geti().ammo;
	if(ammo) {
		if(!player->wears[Quiver].is(ammo))
			return; // No Ammo!
		attack_damage.b += ammo->damage.b;
		if(player->wears[Quiver].is(Precise))
			chance_critical++;
		// Use ammo
		player->wears[Quiver].consume();
	}
	if(weapon.is(Precise))
		chance_critical++;
	// Magical weapon stats
	auto magic_bonus = power.counter;
	bonus += magic_bonus;
	attack_damage.b += magic_bonus;
	// Other stats
	auto ac = defender->get(AC);
	if(!isrange) {
		if((player->is(ChaoticEvil) || player->is(Undead)) && defender->is(ProtectedFromEvil))
			ac += 2;
		// RULE: Small dwarf use special tactics vs large opponents
		if(player->is(Large) && defender->is(BonusACVsLargeEnemy))
			ac += 4;
	}
	if(player->hate.is(defender->race)) {
		if(player->is(BonusAttackVsHated))
			bonus += 1;
		if(player->is(BonusDamageVsEnemy))
			bonus += 4;
	}
	//if(wi.weapon) {
	//	magic_bonus = wi.weapon->getmagic();
	//	if(defender->is(Undead)) {
	//		auto holyness = wi.weapon->getenchant(OfHolyness);
	//		bonus += holyness;
	//		wi.damage.b += holyness * 2;
	//		if(wi.weapon->is(SevereDamageUndead))
	//			wi.damage.b += 2;
	//	}
	//}
	auto tohit = 20 - bonus - (10 - ac);
	auto rolls = xrand(1, 20);
	auto hits = -1;
	tohit = imax(2, imin(20, tohit));
	is_critical_hit = false;
	if(rolls >= tohit || rolls >= chance_critical) {
		// If weapon hits
		if(rolls >= tohit && rolls >= chance_critical) {
			// RULE: crtitical hit can apply only if attack hit and can be deflected
			if(!defender->roll(CriticalDeflect))
				is_critical_hit = true;
		}
		if(is_critical_hit) {
			multiplier += 1;
			if(weapon.is(Deadly))
				multiplier += 1;
		}
		attack_damage.m = multiplier;
		hits = attack_damage.roll();
		// Weapon of specific damage type
		if(power.iskind<damagei>()) {
			damage_type = (damagen)power.value;
			switch(power.value) {
			case Fire: hits += xrand(1, 6); break;
			case Cold: hits += xrand(2, 5); break;
			default: hits += xrand(0, 2); break;
			}
		}
	}
	// Show result
	defender->damage(damage_type, hits, magic_bonus);
	fix_attack(player, slot, hits);
	if(hits != -1) {
		// After all effects, if hit, do additional effects
		if(is_critical_hit) {
			// RULE: Weapon with spell cast it when critical hit occurs
			if(power.iskind<spelli>()) {
				//	auto spell = (spell_s)power.value;
				//	if(bsdata<spelli>::elements[spell].effect.type.type == Damage)
				//		cast(spell, Mage, wi.weapon->getmagic(), defender);
				//	else
				//		cast(spell, Mage, wi.weapon->getmagic(), this);
			}
		}
		// RULE: vampiric ability allow user to drain blood and regain own HP
		if(player->is(weapon, VampiricAttack)) {
			auto hits_healed = xrand(1, 3);
			if(hits_healed > hits)
				hits_healed = hits;
			player->heal(hits_healed);
		}
		// RULE: diseased weapon can cause disease if hit
		if(player->is(weapon, DiseaseAttack)) {
			if(!defender->roll(SaveVsPoison))
				defender->add(DiseaseLevel, 1);
		}
		// RULE: Drain attacks
		if(player->is(weapon, DrainStrenghtAttack) && !defender->roll(SaveVsMagic))
			defender->add(DrainStrenght, 1);
		//		// Poison attack
		//		if(wi.is(OfPoison))
		//			defender->add(Poison, Instant, SaveNegate);
		//		// Paralize attack
		//		if(wi.is(OfParalize))
		//			defender->add(HoldPerson, xrand(1, 3), SaveNegate);
		//		// Drain ability
		//		if(wi.is(OfEnergyDrain))
		//			attack_drain(defender, defender->drain_energy, hits);
		//		defender->damage(damage_type, hits, magic_bonus);
	}
	// Weapon can be broken
	if(rolls == 1) {
		if(weapon && d100() < 60) {
			auto name = weapon.getname();
			weapon.damage(1);
			if(!weapon)
				player->speak("Weapon", "Broken", name);
		} else
			player->damage(Bludgeon, 1, 3);
		return;
	}
}

static void single_main_attack(wearn wear, creaturei* enemy, int bonus, int multiplier) {
	auto number_attacks = 2;
	if(player->is(WeaponSpecialist) && player->isspecialist(&player->wears[wear].geti())) {
		bonus += 1;
		number_attacks += 1;
	}
	if(party.abilities[Minutes] % 2)
		number_attacks += 1;
	number_attacks /= 2;
	while(number_attacks-- > 0)
		single_attack(enemy, wear, bonus, multiplier);
}

static void make_full_attack(creaturei* enemy, int bonus, int multiplier) {
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
	// RULE: sneak attack depend on move silently check and invisibility
	if(enemy->is(Surprised) || player->is(Invisibled)) {
		auto theif = player->get(Theif);
		if(theif > 0 && player->roll(MoveSilently)) {
			consolen(getnm("SneakAttackAct"));
			multiplier += (theif + 7) / 4;
		}
	}
	if(wp2) {
		single_main_attack(RightHand, enemy, bonus + player->gethitpenalty(-4), multiplier);
		single_attack(enemy, LeftHand, bonus + player->gethitpenalty(-6), multiplier);
	} else
		single_main_attack(RightHand, enemy, bonus, multiplier);
	if(wp3)
		single_attack(enemy, Head, bonus, multiplier);
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

void make_attacks(bool melee_combat) {
	if(melee_combat) {
		auto v = to(party, party.d);
		auto d = to(party.d, Down);
		turnto(v, d);
		animation_update();
		enemy_distance = 1;
		if(!select_combatants(v))
			return;
	} else {
		if(!select_combatants(party, party.d))
			return;
	}
	auto push_player = player;
	auto d = to(party.d, Down);
	for(auto p : combatants) {
		player = p;
		if(player->isdisabled())
			continue;
		player->set(Moved);
		// RULE: Surprised creatures do not move first round in combat
		if(player->is(Surprised)) {
			player->remove(Surprised);
			continue;
		}
		// If we can only shoot
		if(enemy_distance > 1 && !player->wears[RightHand].isranged())
			continue;
		if(player->ismonster()) {
			auto left_side = (get_side(player->side, d) % 2) == 0;
			if(player->is(Large))
				left_side = (rand() % 2);
			make_full_attack(get_opponent(left_side, false), 0, 1);
		} else {
			auto left_side = (player->side % 2) == 0;
			make_full_attack(get_opponent(left_side, true), 0, 1);
		}
		animation_update();
		fix_animate();
	}
	player = push_player;
}