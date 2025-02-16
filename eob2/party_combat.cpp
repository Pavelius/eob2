#include "adat.h"
#include "answers.h"
#include "boost.h"
#include "cell.h"
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
		if(p->is(SlowMove))
			p->initiative = 30 + xrand(1, 10);
		else if(p->is(Surprised))
			p->initiative = 100; // Move last
		else
			p->initiative = xrand(1, 10) + p->get(Speed);
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

static void drain_attack(creaturei* defender, const item& weapon, featn type, abilityn ability, int save_bonus) {
	if(!player->is(weapon, type))
		return;
	if(player->is(Undead) && defender->is(ProtectedFromEvil))
		return;
	if(defender->roll(SaveVsMagic, save_bonus))
		return;
	defender->add(ability, 1);
	auto drain_death = (defender->get(DrainLevel) >= defender->getlevel())
		|| (defender->get(DrainStrenght) >= defender->basic.abilities[Strenght])
		|| (defender->get(DrainConstitution) >= defender->basic.abilities[Constitution]);
	if(drain_death) {
		if(defender->ismonster())
			defender->kill();
		else
			defender->hp = -10;
	}
}

static void hit_equipment(creaturei* player) {
	static wearn equipment[] = {Body, LeftHand, Head, Elbow, Legs, Neck};
	if(player->isdisabled())
		return;
	for(auto w : equipment) {
		if(!player->wears[w] || player->wears[w].is(You))
			continue;
		if(player->roll(SaveVsParalization))
			continue;
		player->wears[w].damage(0, 1);
		break;
	}
}

static void single_attack(creaturei* defender, wearn slot, int bonus, int multiplier) {
	if(!defender)
		return;
	auto& weapon = player->wears[slot];
	if(!weapon.isweapon())
		return;
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
	// Other stats
	auto ac = defender->get(AC);
	if(!isrange) {
		if((player->is(ChaoticEvil) || player->is(Undead)) && defender->is(ProtectedFromEvil))
			ac += 1;
		// RULE: Small dwarf use special tactics vs large opponents
		if(player->is(Large) && defender->is(BonusACVsLargeEnemy))
			ac += 4;
	}
	// RULE: Magically blurred wizard
	if((defender->is(Blurred) || defender->is(Invisibled)) && !player->is(ImmuneIllusion))
		bonus -= 4;
	if((player->is(Invisibled) && !defender->is(ImmuneIllusion)) || defender->is(Surprised))
		bonus += 4;
	// RULE: One race hate another
	if(player->hate.is(defender->race)) {
		if(player->is(BonusAttackVsHated))
			bonus += 1;
		if(player->is(BonusDamageVsEnemy))
			bonus += 4;
	}
	if((power.iskind<racei>() && defender->race == power.value)
		|| (player->is(weapon, Holy) && defender->is(Undead))) {
		bonus += 3;
		multiplier += 1;
	}
	if(player->is(Paniced))
		bonus -= 2;
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
		if(player->is(weapon, VorpalAttack) && is_critical_hit)
			hits = 1000;
		if(player->is(weapon, DispelEvilAttack) && defender->is(Outsider)) {
			if(!defender->roll(SaveVsMagic))
				hits = 1000;
		}
		if(defender->is(Displaced) && d100() < 50)
			hits = -1; // Miss if displaced
		if(defender->is(Blinked) && defender->initiative < player->initiative)
			hits = -1; // Miss if blinked away
	}
	// Show result
	if(!player->is(ImmuneIllusion) && defender->get(DuplicateIllusion)) {
		hits = -2; // RULE: Mirror image effect
		defender->add(DuplicateIllusion, -1);
	} else
		defender->damage(damage_type, hits, magic_bonus);
	fix_attack(player, slot, hits);
	if(hits > 0) {
		// After all effects, if hit, do additional effects
		if(is_critical_hit) {
			// RULE: Weapon with spell cast it when critical hit occurs
			if(slot == RightHand && power.iskind<spelli>()) {
				auto ps = bsdata<spelli>::elements + power.value;
				if(ps->is(Enemy))
					cast_spell(ps, player->getlevel(), 0, true, false, 0, defender);
				else
					cast_spell(ps, player->getlevel(), 0, true, false, 0, player);
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
		// RULE: poison attack
		if(player->is(weapon, PoisonAttack)) {
			if(!defender->roll(SaveVsPoison))
				defender->add(PoisonLevel, xrand(2, 8));
		}
		// RULE: paralized attack of ghouls and others
		if(player->is(weapon, ParalizeAttack) && !defender->roll(SaveVsParalization))
			add_boost(get_stamp(xrand(3, 8)), defender, BoostFeat, Paralized);
		drain_attack(defender, weapon, DrainStrenghtAttack, DrainStrenght, 0);
		drain_attack(defender, weapon, DrainEneryAttack, DrainLevel, -100);
		// Poison attack
		//if(wi.is(OfPoison))
		// defender->add(Poison, Instant, SaveNegate);
		// 15% of all attack can damage equipment (if e hit and can harm)
		if(d100() < 15)
			hit_equipment(defender);
	}
	// Weapon can be broken
	if(rolls == 1)
		weapon.damage("WeaponBroken", 1);
}

static void single_main_attack(wearn wear, creaturei* enemy, int bonus, int multiplier) {
	auto number_attacks = player->wears[wear].geti().number_attacks;
	if(!number_attacks)
		number_attacks = 2;
	number_attacks += player->get(AdditionalAttacks);
	if(player->is(WeaponSpecialist) && player->isspecialist(&player->wears[wear].geti()))
		number_attacks += 1;
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
	// RULE: backstabbing attack depend on surprise check and invisibility. Chance is move silently.
	if(enemy->is(Surprised) || player->is(Invisibled) || player->initiative < enemy->initiative) {
		auto theif_bakstab = player->get(Backstab);
		if(theif_bakstab > 0 && player->roll(MoveSilently)) {
			consolen(getnm("SneakAttackAct"));
			multiplier += theif_bakstab;
			if(!bonus)
				bonus += 4;
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

void turnto(pointc v, directions d, bool test_surprise) {
	if(!d)
		return;
	if(v == party) {
		if(test_surprise) {
			if(party.d != d) {
				creaturei* monsters[6]; loc->getmonsters(monsters, to(v, d));
				surprise_roll(characters, party_sneaky(monsters));
			}
		}
		set_party_position(v, d);
	} else {
		creaturei* monsters[6] = {}; loc->getmonsters(monsters, v, party.d);
		for(int i = 0; i < 6; i++) {
			auto p = monsters[i];
			if(!p || p->isdisabled())
				continue;
			if(test_surprise) {
				if(p->d != d) {
					surprise_roll(monsters, party_sneaky(characters));
					test_surprise = false;
				}
			}
			p->d = d;
		}
	}
}

static int get_hit_points(celln t) {
	switch(t) {
	case CellWeb: return 4; // Easy to hit. Affected by fire spells.
	case CellCocon: return 5;
	case CellBarel: return 7; // Tought to hit. Crushed by acid spells.
	case CellEyeColumn: return 10;
	default: return 0;
	}
}

bool make_object_attack(pointc v) {
	if(!player || !player->isready())
		return false;
	auto slot = RightHand;
	auto object = loc->get(v);
	auto toughness = get_hit_points(object);
	if(!toughness)
		return false;
	auto bonus = 0;
	auto damage = player->getdamage(bonus, slot, false);
	auto tohit = 20 - bonus - 10;
	auto rolls = xrand(1, 20);
	auto hits = -1;
	tohit = imax(2, imin(20, tohit));
	if(rolls >= tohit) {
		hits = damage.roll();
		if(hits < toughness)
			hits = -1;
	}
	fix_attack(player, slot, hits);
	if(hits > 0)
		broke_cell(v);
	return true;
}

static spelli* ai_choose_spell() {
	pushanswer push;
	auto max_spells = sizeof(player->spells) / sizeof(player->spells[0]);
	for(size_t i = 0; i < max_spells; i++) {
		if(!player->spells[i])
			continue;
		auto ps = bsdata<spelli>::elements + i;
		if(!cast_spell(ps, player->getlevel(), 32, false, false, 0, 0))
			continue;
		an.add(ps, ps->getname());
	}
	if(!an)
		return 0;
	return (spelli*)an.random();
}

static bool ai_use_spells() {
	auto ps = ai_choose_spell();
	if(!ps)
		return false;
	cast_spell(ps, player->getlevel(), 35, true, true, 0, 0);
	return true;
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
		player->set(Moved);
		if(!player->isready())
			continue;
		// RULE: Surprised creatures do not move first round in combat
		if(player->is(Surprised)) {
			player->remove(Surprised);
			continue;
		}
		// RULE: Paniced
		if(player->is(Paniced)) {
			if(d100() < 30) {
				player->speak("Paniced", "Apply");
				continue;
			}
		}
		// If we can only shoot
		if(enemy_distance > 1 && !player->wears[RightHand].isranged())
			continue;
		if(player->ismonster()) {
			if(!ai_use_spells()) {
				auto left_side = (get_side(player->side, d) % 2) == 0;
				if(player->is(Large))
					left_side = (rand() % 2);
				make_full_attack(get_opponent(left_side, false), 0, 1);
			}
		} else {
			auto left_side = (player->side % 2) == 0;
			make_full_attack(get_opponent(left_side, true), 0, 1);
		}
		animation_update();
		fix_animate();
	}
	player = push_player;
}
