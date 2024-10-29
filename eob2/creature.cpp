#include "advancement.h"
#include "alignment.h"
#include "avatar.h"
#include "boost.h"
#include "bsdata.h"
#include "class.h"
#include "console.h"
#include "creature.h"
#include "direction.h"
#include "dungeon.h"
#include "gender.h"
#include "list.h"
#include "math.h"
#include "modifier.h"
#include "monster.h"
#include "pushvalue.h"
#include "race.h"
#include "rand.h"
#include "speech.h"
#include "script.h"
#include "slice.h"
#include "stringbuilder.h"
#include "party.h"
#include "view.h"

creaturei *player, *opponent;
int last_roll, last_chance;
classn last_class;
racen last_race;

static char hit_points_adjustment[] = {
	-4, -3, -2, -2, -1, -1, -1, 0, 0, 0,
	0, 0, 0, 0, 0, 1, 2, 3, 4, 5,
	5, 6, 6, 6, 7, 7
};
static char reaction_adjustment[] = {
	-7, -6, -4, -3, -2, -1, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 1, 2, 2, 3, 3,
	4, 4, 4, 5, 5
};
static char defence_adjustment[] = {
	-5, -5, -4, -3, -2, -1, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 1, 2, 3, 4, 4,
	4, 5, 5, 5, 6, 6
};
static char hit_probability[] = {
	-5, -5, -3, -3, -2, -2, -1, -1, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 1, 1,
	1, 2, 2, 2, 3,
	3, 4, 4, 5, 6, 7
};
static char damage_adjustment[] = {
	-5, -5, -3, -3, -2, -2, -1, -1, 0, 0,
	0, 0, 0, 0, 0, 0, 1, 1, 2,
	3, 3, 4, 5, 6,
	7, 8, 9, 10, 11, 12, 14
};
static char cha_reaction_adjustment[] = {
	-10, -7, -6, -5, -4, -3, -2, -1, 0, 0,
	0, 0, 0, 1, 2, 3, 5, 6, 7,
	8, 9, 10, 11, 12, 13, 14
};
static char dwarven_bonus[] = {
	0, 0, 0, 0, 1, 1, 1, 2, 2, 2,
	2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5,
	6, 6, 6, 6, 7
};
static const int hd_experience[] = {
	7, 15, 35, 65, 120, 175, 270, 420, 650, 975,
	1400, 1700, 2000, 3000
};
//static char open_doors[] = {
//	18, 20, 22, 26, 28, 30, 32, 34, 36, 38,
//	40, 42, 44, 46, 48, 50, 54, 58, 62,
//	66, 72, 78, 84, 90,
//	92, 94, 95
//};
//static char bend_bars[] = {
//	0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
//	2, 2, 4, 4, 7, 7, 10, 13, 16,
//	20, 25, 30, 35, 40, 45, 50,
//};
//static char system_shock_survival[] = {
//	30, 30, 30, 35, 40, 45, 50, 55, 60, 65,
//	70, 75, 80, 85, 88, 91, 95, 97, 99
//};

void creaturei::clear() {
	memset(this, 0, sizeof(*this));
	avatar = 0xFF;
	name = 0xFFFF;
	monster_id = 0xFFFF;
	x = -1;
	y = -1;
}

static void update_languages() {
	player->languages = player->getrace().languages;
	player->understand(player->race);
	if(player->getrace().origin)
		player->understand(player->getrace().origin);
	if(player->basic.abilities[Intellegence] >= 11)
		player->understand((racen)0); // All creatures with 11+ untellegence known common language
}

static void update_basic() {
	memcpy(player->abilities, player->basic.abilities, ExeptionalStrenght + 1);
	memcpy(player->feats, player->basic.feats, sizeof(player->basic.feats));
}

static int get_maximum_hits() {
	auto n = bsdata<classi>::elements[player->type].count;
	auto m = player->getlevel();
	auto a = player->get(Constitution);
	auto h = maptbl(hit_points_adjustment, a);
	if(h > 2 && !player->is(BonusHP))
		h = 2;
	auto r = player->get(Hits) + h * m + player->hpr / imax(1, (int)n);
	if(r < m)
		r = m;
	return r;
}

static int get_modified_strenght() {
	auto a = player->get(Strenght);
	auto e = player->get(ExeptionalStrenght);
	if(player->is(NoExeptionalStrenght))
		e = 0;
	if(a > 18)
		a += 6;
	else if(a == 18 && e > 0) {
		if(e <= 50)
			a += 1;
		else if(e <= 75)
			a += 2;
		else if(e <= 90)
			a += 3;
		else if(e <= 99)
			a += 4;
		else
			a += 5;
	}
	return a;
}

static void update_abilities() {
	player->add(Strenght, -player->abilities[DrainStrenght]);
	player->add(AttackMelee, -player->abilities[DrainStrenght]);
	player->add(AttackRange, -player->abilities[DrainStrenght]);
	player->add(Constitution, -player->abilities[DrainConstitution]);
}

static void add_additional_spell(abilityn v) {
	if(player->abilities[v])
		player->abilities[v]++;
}

static void update_additional_spells() {
	auto k = player->get(Wisdow);
	if(k >= 13)
		add_additional_spell(Spell1);
	if(k >= 14)
		add_additional_spell(Spell1);
	if(k >= 15)
		add_additional_spell(Spell2);
	if(k >= 16)
		add_additional_spell(Spell2);
	if(k >= 17)
		add_additional_spell(Spell3);
	if(k >= 18)
		add_additional_spell(Spell4);
}

static void update_depended_abilities() {
	auto k = get_modified_strenght();
	player->abilities[AttackMelee] += maptbl(hit_probability, k);
	player->abilities[AttackRange] += maptbl(reaction_adjustment, player->abilities[Dexterity]);
	player->abilities[DamageMelee] += maptbl(damage_adjustment, k);
	player->abilities[AC] += maptbl(defence_adjustment, player->abilities[Dexterity]);
	player->abilities[ReactionBonus] += maptbl(cha_reaction_adjustment, player->abilities[Charisma]);
	if(player->wears[RightHand])
		player->abilities[Speed] -= player->wears[RightHand].geti().speed;
	else
		player->abilities[Speed] -= player->wears[LeftHand].geti().speed;
}

static void update_bonus_saves() {
	auto k = player->get(Constitution);
	if(player->is(BonusSaveVsPoison))
		player->abilities[SaveVsPoison] += maptbl(dwarven_bonus, k) * 5;
	if(player->is(BonusSaveVsSpells))
		player->abilities[SaveVsMagic] += maptbl(dwarven_bonus, k) * 5;
}

static void update_wear() {
	auto push_modifier = modifier;
	modifier = Standart;
	for(auto& e : player->equipment()) {
		if(!e)
			continue;
		auto& ei = e.geti();
		if(!ei.wearing)
			continue;
		if(ei.wear == LeftHand) {
			if(player->wears[RightHand] && player->wears[RightHand].is(TwoHanded))
				continue; // RULE: Two handed weapon
		}
		script_run(ei.wearing);
	}
	modifier = push_modifier;
}

static void update_duration() {
	auto push_modifier = modifier; modifier = Standart;
	referencei target = player;
	for(auto& e : bsdata<boosti>()) {
		if(e.target == target)
			script_run(e.effect);
	}
	modifier = push_modifier;
}

static bool have_boost_summon(const item& it) {
	referencei target = player;
	for(auto& e : bsdata<boosti>()) {
		if(e.target == target && e.effect.iskind<spelli>()) {
			auto& ei = bsdata<spelli>::elements[e.effect.value];
			if(it.is(ei.summon))
				return true;
		}
	}
	return false;
}

static void update_summon() {
	for(auto& it : player->wears) {
		if(it.issummoned() && !have_boost_summon(it))
			it.clear();
	}
}

void update_player() {
	update_basic();
	update_languages();
	update_summon();
	update_wear();
	update_duration();
	update_abilities();
	update_depended_abilities();
	update_additional_spells();
	update_bonus_saves();
	player->hpm = get_maximum_hits();
}

static int compare_char_desc(const void* v1, const void* v2) {
	return *((char*)v2) - *((char*)v1);
}

static int get_best_4d6() {
	char result[4];
	for(size_t i = 0; i < sizeof(result) / sizeof(result[0]); i++)
		result[i] = (rand() % 6) + 1;
	qsort(result, sizeof(result) / sizeof(result[0]), sizeof(result[0]), compare_char_desc);
	return result[0] + result[1] + result[2];
}

static int get_best_index(char* result, size_t size) {
	auto result_index = 0;
	for(size_t i = 0; i < size; i++) {
		if(result[i] > result[result_index])
			result_index = i;
	}
	return result_index;
}

static void apply_minimal(char* abilities, const char* minimal) {
	for(auto i = 0; i < 6; i++) {
		if(minimal[i] && abilities[Strenght + i] < minimal[i])
			abilities[Strenght + i] = minimal[i];
	}
}

static void apply_maximal(char* abilities, const char* maximal) {
	for(auto i = 0; i < 6; i++) {
		if(maximal[i] && abilities[Strenght + i] > maximal[i])
			abilities[Strenght + i] = maximal[i];
	}
}

static void generate_abilities() {
	auto pc = bsdata<classi>::elements + player->type;
	auto pr = bsdata<racei>::elements + player->race;
	char result[12] = {};
	if(true) {
		for(size_t i = 0; i < sizeof(result) / sizeof(result[0]); i++)
			result[i] = (rand() % 6) + (rand() % 6) + (rand() % 6) + 3;
		qsort(result, sizeof(result) / sizeof(result[0]), sizeof(result[0]), compare_char_desc);
		zshuffle(result, 6);
	} else {
		for(size_t i = 0; i < 6; i++)
			result[i] = get_best_4d6();
	}
	for(size_t i = 0; i < 6; i++)
		player->basic.abilities[Strenght + i] = result[i];
	iswap(player->basic.abilities[get_best_index(player->basic.abilities + Strenght, 6)], player->basic.abilities[pc->primary]);
	apply_minimal(player->basic.abilities, pc->minimal);
	apply_minimal(player->basic.abilities, pr->minimal);
	apply_maximal(player->basic.abilities, pr->maximal);
	player->basic.abilities[ExeptionalStrenght] = d100() + 1;
}

static void advance_level(variant id, int level) {
	id.counter = level;
	auto push_modifier = modifier;
	modifier = Permanent;
	for(auto& e : bsdata<advancement>()) {
		if(e.type == id)
			script_run(e.elements);
	}
	modifier = push_modifier;
}

static void set_class_ability() {
	auto pc = bsdata<classi>::elements + player->type;
	advance_level(pc, 0);
	// Roll for hits first time
	for(char i = 0; i < pc->count; i++) {
		auto pd = bsdata<classi>::elements + pc->classes[i];
		player->levels[i]++;
		if(pd->hd) {
			player->hpr = 1 + rand() % pd->hd;
			if(player->hpr < pd->hd / 2)
				player->hpr = pd->hd / 2;
		}
		advance_level(pd, player->levels[i]);
	}
}

static void set_race_ability() {
	advance_level(bsdata<racei>::elements + player->race, 0);
}

static void set_starting_equipment() {
	auto pc = bsdata<classi>::elements + player->type;
	auto pr = bsdata<racei>::elements + player->race;
	auto p = bsdata<listi>::find(ids(pr->id, pc->id, "StartEquipment"));
	if(!p)
		p = bsdata<listi>::find(ids(pc->id, "StartEquipment"));
	if(p)
		script_run(p->elements);
}

static bool no_party_avatars(unsigned char value) {
	for(auto i = 0; i < 6; i++) {
		if(characters[i] && characters[i]->avatar == value)
			return false;
	}
	return true;
}

static bool is_party_name(unsigned short value) {
	for(auto i = 0; i < 6; i++) {
		if(characters[i] && characters[i]->name == value)
			return true;
	}
	return false;
}

static void set_basic_ability() {
	player->basic.abilities[Alertness] += 70;
}

void create_player() {
	player = bsdata<creaturei>::add();
	player->clear();
	create_npc(player, no_party_avatars, is_party_name);
	generate_abilities();
	set_basic_ability();
	set_race_ability();
	set_class_ability();
	update_player();
	set_starting_equipment();
	update_player();
	player->hp = player->hpm;
}

static void apply_feats(const variants& elements) {
	auto push_modifier = modifier;
	modifier = Permanent;
	script_run(elements);
	modifier = push_modifier;
}

static void raise_monster_level() {
	if(player->levels[0]) {
		for(auto i = 1; i <= player->levels[0]; i++) {
			advance_level(bsdata<classi>::elements + player->type, i);
			player->hpr = xrand(1, bsdata<classi>::elements[player->type].hd);
		}
	} else
		player->hpr = xrand(1, 6);
}

static void apply_default_ability() {
	for(auto i = Strenght; i <= Charisma; i = (abilityn)(i + 1))
		player->basic.abilities[i] = 10;
}

void create_monster(const monsteri* pi) {
	if(!pi)
		return;
	player->clear();
	player->monster_id = getbsi(pi);
	player->levels[0] = pi->hd;
	player->basic.abilities[AC] = (10 - pi->ac);
	set_basic_ability();
	apply_default_ability();
	apply_feats(pi->feats);
	raise_monster_level();
	update_player();
	player->hp = player->hpm;
}

const char*	creaturei::getname() const {
	auto pm = getmonster();
	if(pm)
		return pm->getname();
	return npc::getname();
}

creaturei* item_owner(const void* p) {
	auto i = bsdata<creaturei>::source.indexof(p);
	if(i != -1 && p != (bsdata<creaturei>::elements + i))
		return (creaturei*)bsdata<creaturei>::elements + i;
	return 0;
}

wearn item_wear(const void* p) {
	auto i = bsdata<creaturei>::source.indexof(p);
	if(i != -1) {
		auto pi = (creaturei*)bsdata<creaturei>::elements + i;
		if(p >= pi->wears && p <= pi->wears + LastBelt)
			return (wearn)((item*)p - pi->wears);
	}
	return Backpack;
}

bool creaturei::isallow(const item& it) const {
	auto item_feats = it.geti().feats[0];
	auto player_feats = feats[0];
	// One of this
	const unsigned m0 = FG(UseMartial) | FG(UseElvish) | FG(UseRogish) | FG(UsePriest) | FG(UseMage);
	auto v0 = item_feats & m0;
	if(v0 && (v0 & (player_feats & m0)) == 0)
		return false;
	// All of this
	const unsigned m1 = FG(UseMetal) | FG(UseLeather) | FG(UseShield);
	auto v1 = item_feats & m1;
	if((v1 & (player_feats & m1)) != v1)
		return false;
	return true;
}

void creaturei::additem(item& it) {
	if(isallow(it))
		equip(it);
	if(it)
		wearable::additem(it);
}

dice creaturei::getdamage(int& bonus, wearn id, bool large_enemy) const {
	auto& ei = wears[id].geti();
	dice result = ei.damage;
	if(large_enemy && ei.damage_large)
		result = ei.damage_large;
	auto isranged = wears[id].isranged();
	bonus += player->get(isranged ? AttackRange : AttackMelee);
	result.b += player->get(isranged ? DamageRange : DamageMelee);
	if(is(WeaponSpecialist) && isspecialist(&ei)) {
		if(wears[id].isranged())
			bonus += 2;
		else {
			bonus += 1;
			result.b += 2;
		}
	}
	return result;
}

int	creaturei::getchance(abilityn v) const {
	if(v >= Strenght && v <= Charisma)
		return abilities[v] * 5;
	else if(v >= SaveVsParalization && v <= DamageRange)
		return abilities[v];
	return 0;
}

static bool make_roll(int chance) {
	last_chance = chance;
	if(last_chance <= 0)
		return false;
	if(last_chance >= 100)
		last_chance = 95;
	last_roll = d100();
	return last_roll < last_chance;
}

bool creaturei::roll(abilityn v, int bonus) const {
	return make_roll(getchance(v) + bonus);
}

const char* creaturei::getbadstate() const {
	if(isdead())
		return "Dead";
	else if(isdisabled())
		return "Disabled";
	else if(is(PoisonLevel))
		return "Poisoned";
	else if(is(DiseaseLevel))
		return "Diseased";
	return 0;
}

void creaturei::addexp(int value) {
	experience += value;
}

int creaturei::gethitpenalty(int bonus) const {
	if(is(Precise))
		return 0;
	auto dex = abilities[Dexterity];
	auto bon = maptbl(reaction_adjustment, dex);
	bonus += bon;
	if(bonus > 0)
		bonus = 0;
	return bonus;
}

const monsteri*	creaturei::getmonster() const {
	return getbs<monsteri>(monster_id);
}

bool can_remove(const item* pi, bool speech) {
	auto player = item_owner(pi);
	if(!player)
		return true;
	auto w = item_wear(pi);
	if(w >= Head && w <= Quiver) {
		if(pi->iscursed()) {
			if(speech)
				player->say(speech_get("CantDropCursedItem"));
			return false;
		}
	}
	return true;
}

void creaturei::setframe(short* frames, short index) const {
	if(ismonster()) {
		auto po = getmonster()->overlays;
		frames[0] = po[0] * 6 + index;
		frames[1] = po[1] ? po[1] * 6 + index : 0;
		frames[2] = po[2] ? po[2] * 6 + index : 0;
		frames[3] = po[3] ? po[3] * 6 + index : 0;
	} else {
		frames[0] = index;
		frames[1] = 0;
	}
}

bool creaturei::isactable() const {
	if(isdisabled()) {
		consolen("%1 is %2", getname(), getnm(getbadstate()));
		return false;
	}
	return true;
}

void creaturei::damage(damagen type, int value, char magic_bonus) {
	if(value <= 0)
		return;
	switch(type) {
	case Poison:
		consolen(getnm(ids("Feel", bsdata<damagei>::elements[type].id)));
		break;
	default:
		fix_damage(this, value);
		break;
	}
	auto& ei = bsdata<damagei>::elements[type];
	if(ei.resist && is(ei.resist))
		value = value / 2;
	else if(ei.immunity && is(ei.immunity))
		value = 0;
	hp -= value;
	if(hp <= 0)
		kill();
}

int creaturei::getexpaward() const {
	auto pm = getmonster();
	auto r = getlevel();
	if(!pm)
		return r * 100;
	if(pm->experience)
		return pm->experience;
	if(get(AC) >= 10)
		r += 1;
	//if(is(OfPoison))
	//	r += 1;
	if(is(ResistBludgeon) || is(ResistPierce) || is(ResistSlashing))
		r += 1;
	if(is(ImmuneNormalWeapon))
		r += 1;
	//if(is(OfEnergyDrain))
	//	r += 3;
	if(is(Undead))
		r++;
	//if(is(OfFear))
	//	r += 1;
	//if(is(OfParalize))
	//	r += 2;
	if(basic.abilities[ResistMagic] >= 50)
		r++;
	if(basic.abilities[ResistMagic] >= 90)
		r++;
	auto exp = maptbl(hd_experience, r);
	if(r > 13)
		exp += (r - 13) * 1000;
	return exp;
}

static void drop_loot(creaturei* player) {
	for(auto& it : player->wears) {
		if(!it || it.isnatural())
			continue;
		if(it.is(Unique) || (d100() < 15)) {
			it.identify(0);
			loc->drop(*player, it, get_side(player->side, party.d));
		}
	}
}

void creaturei::kill() {
	if(!ismonster())
		return;
	// auto hitd = getlevel();
	party_addexp(getexpaward());
	party_addexp_per_killed(getlevel());
	drop_loot(this);
	loc->state.monsters_killed++;
	clear();
}

static bool isf(const creaturei* player, const item& weapon, featn v) {
	if(player->is(v) || weapon.geti().is(v))
		return true;
	return false;
}

void creaturei::attack(creaturei* defender, wearn slot, int bonus, int multiplier) {
	auto& weapon = wears[slot];
	auto power = weapon.getpower();
	auto chance_critical = 20;
	auto attack_damage = getdamage(bonus, slot, defender->is(Large));
	auto damage_type = weapon.geti().damage_type;
	auto isrange = weapon.isranged();
	auto ammo = weapon.geti().ammo;
	if(ammo) {
		if(!wears[Quiver].is(ammo))
			return; // No Ammo!
		attack_damage.b += ammo->damage.b;
		if(wears[Quiver].is(Precise))
			chance_critical++;
		// Use ammo
		wears[Quiver].consume();
	}
	if(weapon.is(Precise))
		chance_critical++;
	// Magical weapon stats
	auto magic_bonus = power.counter;
	bonus += magic_bonus;
	attack_damage.b += magic_bonus;
	// Other stats
	auto ac = defender->get(AC);
	// RULE: Small dwarf use special tactics vs large opponents
	if(!isrange) {
		if(defender->is(BonusACVsLargeEnemy) && is(Large))
			ac += 4;
	}
	if(hate.is(defender->race)) {
		if(is(BonusAttackVsHated))
			bonus += 1;
		if(is(BonusDamageVsEnemy))
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
	if(isf(this, weapon, BloodSucking) && is(Assembled))
		rolls = tohit;
	if(rolls >= tohit || rolls >= chance_critical) {
		// If weapon hits
		auto is_critical = false;
		if(rolls >= tohit && rolls >= chance_critical) {
			// RULE: crtitical hit can apply only if attack hit and can be deflected
			if(!defender->roll(CriticalDeflect))
				is_critical = true;
		}
		if(is_critical) {
			multiplier += 1;
			if(weapon.is(Deadly))
				multiplier += 1;
		}
		if(isf(this, weapon, BloodSucking) && is(Assembled))
			hits = attack_damage.maximum();
		else
			hits = attack_damage.roll();
		hits = hits * multiplier;
		// Weapon of specific damage type
		if(power.iskind<damagei>()) {
			switch(power.value) {
			case Fire: hits += xrand(1, 6); break;
			case Cold: hits += xrand(2, 5); break;
			default: hits += xrand(1, 3); break;
			}
			damage_type = (damagen)power.value;
		}
		if(is_critical) {
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
		if(isf(this, weapon, VampiricAttack)) {
			auto hits_healed = xrand(2, 5);
			if(hits_healed > hits)
				hits_healed = hits;
		}
		set(Assembled); // Fix assemble to victium
		if(is(Disease))
			defender->abilities[DiseaseLevel]++;
	}
	// Show result
	defender->damage(damage_type, hits, magic_bonus);
	fix_attack(this, slot, hits);
	if(hits != -1) {
		// Drain attacks
		if(isf(player, weapon, DrainStrenghtAttack) && !defender->roll(SaveVsMagic))
			defender->abilities[DrainStrenght]++;
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
				speak("Weapon", "Broken", name);
		} else
			damage(Bludgeon, 1, 3);
		return;
	}
}

const racei& creaturei::getrace() const {
	return bsdata<racei>::elements[type];
}

const classi& creaturei::getclass() const {
	return bsdata<classi>::elements[type];
}

const classi& creaturei::getclassmain() const {
	return bsdata<classi>::elements[bsdata<classi>::elements[type].classes[0]];
}

void creaturei::add(abilityn i, int v) {
	if(v >= 0) {
		switch(i) {
		case DiseaseLevel:
			if(is(ImmuneDisease))
				return;
			break;
		}
	}
	statable::add(i, v);
}