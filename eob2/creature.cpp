#include "advancement.h"
#include "avatar.h"
#include "bsdata.h"
#include "class.h"
#include "creature.h"
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

creaturei* player;
int last_roll, last_chance;

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
	add_value(player->abilities[Strenght], -player->abilities[DrainStrenght]);
	add_value(player->abilities[Constitution], -player->abilities[DrainConstitution]);
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
		script_run(ei.wearing);
	}
	modifier = push_modifier;
}

static void update_duration() {
}

void update_player() {
	update_basic();
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
		if(party.units[i] && party.units[i]->avatar == value)
			return false;
	}
	return true;
}

void create_player(const racei* pr, gendern gender, const classi* pc) {
	if(!pr || !pc)
		return;
	player = bsdata<creaturei>::add();
	player->clear();
	player->race = bsdata<racei>::source.indexof(pr);
	player->gender = gender;
	player->type = bsdata<classi>::source.indexof(pc);
	generate_abilities();
	set_race_ability();
	set_class_ability();
	player->name = generate_name(player->race, player->gender);
	player->avatar = generate_avatar(player->race, gender, player->type, no_party_avatars);
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

void create_monster(const monsteri* pi) {
	if(!pi)
		return;
	player = bsdata<creaturei>::add();
	player->clear();
	apply_feats(pi->feats);
	update_player();
	player->hp = player->hpm;
}

creaturei* item_owner(const void* p) {
	auto i = bsdata<creaturei>::source.indexof(p);
	if(i == -1)
		return 0;
	return (creaturei*)bsdata<creaturei>::elements + i;
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

dice creaturei::getdamage(wearn id) const {
	dice result = wears[id].geti().damage;
	result.b += player->get(DamageMelee);
	return result;
}

int	creaturei::getchance(abilityn v) const {
	if(v >= Strenght && v <= Charisma)
		return abilities[v] * 5;
	else if(v >= SaveVsParalization && v <= DamageRange)
		return abilities[v];
	return 0;
}

bool creaturei::roll(abilityn v, int bonus) const {
	last_chance = getchance(v);
	if(last_chance <= 0)
		return false;
	last_chance += bonus;
	if(last_chance >= 100)
		last_chance = 95;
	last_roll = d100();
	return last_roll < last_chance;
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