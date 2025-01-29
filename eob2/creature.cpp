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

static_assert(sizeof(creaturei::spells) == sizeof(monsteri::spells), "Spells requisit in creaturei and monsteri must be same type.");

const int encounter_table_maximum = 19;
creaturei *player, *opponent, *result_player;
classn last_class;
racen last_race;
bool is_critical_hit;

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
static char chance_learn_spell[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 35,
	40, 45, 50, 55, 60, 65, 70, 75, 85, 95,
	96, 97, 98, 99, 100
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
static char thac0_advance[4][22] = {
	{0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20},
	{0, 0, 0, 0, 2, 2, 2, 4, 4, 4, 6, 6, 6, 8, 8, 8, 10, 10, 10, 12, 12, 12},
	{0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10},
	{0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 6, 6, 6},
};
static char saves_advance[4][5][22] = {
	// Warriors - 0
	{{16, 14, 14, 13, 13, 11, 11, 10, 10, 8, 8, 7, 7, 5, 5, 4, 4, 3, 3, 2, 2, 2},
	{18, 16, 16, 15, 15, 13, 13, 12, 12, 10, 10, 9, 9, 7, 7, 6, 6, 5, 5, 3, 3, 2},
	{17, 15, 15, 14, 14, 12, 12, 11, 11, 9, 9, 8, 8, 6, 6, 5, 5, 4, 4, 3, 3, 2},
	{20, 17, 17, 16, 16, 13, 13, 12, 12, 9, 9, 8, 8, 5, 5, 4, 4, 3, 3, 3, 3, 3},
	{19, 17, 17, 16, 16, 14, 14, 13, 13, 11, 11, 10, 10, 8, 8, 7, 7, 6, 6, 5, 5, 4}},
	// Priest - 1
	{{10, 10, 10, 10, 9, 9, 9, 7, 7, 7, 6, 6, 6, 5, 5, 5, 4, 4, 4, 2, 2, 2},
	{14, 14, 14, 14, 13, 13, 13, 11, 11, 11, 10, 10, 10, 9, 9, 9, 8, 8, 8, 6, 6, 6},
	{13, 13, 13, 13, 12, 12, 12, 10, 10, 10, 9, 9, 9, 8, 8, 8, 7, 7, 7, 5, 5, 5},
	{16, 16, 16, 16, 15, 15, 15, 13, 13, 13, 12, 12, 12, 11, 11, 11, 10, 10, 10, 8, 8, 8},
	{15, 15, 15, 15, 14, 14, 14, 12, 12, 12, 11, 11, 11, 10, 10, 10, 9, 9, 9, 7, 7, 7}},
	// Rogues - 2
	{{13, 13, 13, 13, 13, 12, 12, 12, 12, 11, 11, 11, 11, 10, 10, 10, 10, 9, 9, 9, 9, 8},
	{14, 14, 14, 14, 14, 12, 12, 12, 12, 10, 10, 10, 10, 8, 8, 8, 8, 6, 6, 6, 6, 4},
	{12, 12, 12, 12, 12, 11, 11, 11, 11, 10, 10, 10, 10, 9, 9, 9, 9, 8, 8, 8, 8, 7},
	{16, 16, 16, 16, 16, 15, 15, 15, 15, 14, 14, 14, 14, 13, 13, 13, 13, 12, 12, 12, 12, 11},
	{15, 15, 15, 15, 15, 13, 13, 13, 13, 11, 11, 11, 11, 9, 9, 9, 9, 7, 7, 7, 7, 5}},
	// Wizards - 3
	{{14, 14, 14, 14, 14, 14, 13, 13, 13, 13, 13, 11, 11, 11, 11, 11, 10, 10, 10, 10, 10, 8},
	{11, 11, 11, 11, 11, 11, 9, 9, 9, 9, 9, 7, 7, 7, 7, 7, 5, 5, 5, 5, 5, 3},
	{13, 13, 13, 13, 13, 13, 11, 11, 11, 11, 11, 9, 9, 9, 9, 9, 7, 7, 7, 7, 7, 5},
	{15, 15, 15, 15, 15, 15, 13, 13, 13, 13, 13, 11, 11, 11, 11, 11, 9, 9, 9, 9, 9, 7},
	{12, 12, 12, 12, 12, 12, 10, 10, 10, 10, 10, 8, 8, 8, 8, 8, 6, 6, 6, 6, 6, 4}},
};
static char save_index[] = {
	0, 0,
	1,
	4,
};
static_assert(sizeof(save_index) / sizeof(save_index[0]) == (SaveVsMagic - SaveVsParalization) + 1, "Invalid count of save index elements");

static char theif_skill_adjustment[][5] = {
	{-25, -20, -15, -30, -20}, // 0
	{-25, -20, -15, -30, -20}, // 1
	{-25, -20, -15, -30, -20}, // 2
	{-25, -20, -15, -30, -20}, // 3
	{-25, -20, -15, -30, -20}, // 4
	{-25, -20, -15, -30, -20}, // 5
	{-25, -20, -15, -30, -20}, // 6
	{-25, -20, -15, -30, -20}, // 7
	{-20, -15, -10, -25, -15}, // 8
	{-15, -10, -10, -20, -10}, // 9
	{-10, -5, -10, -15, -5}, // 10
	{-5, 0, -5, -10, 0}, // 11
	{0, 0, 0, -5, 0}, // 12
	{0, 0, 0, 0, 0}, // 13
	{0, 0, 0, 0, 0}, // 14
	{0, 0, 0, 0, 0}, // 15
	{0, 5, 0, 0, 0}, // 16
	{5, 10, 0, 5, 5}, // 17
	{10, 15, 5, 10, 10}, // 18
	{15, 20, 10, 15, 15}, // 19
};

static char theif_skill_basic[][7] = {
	{40, 10, 0, 0, 0, 0, 0}, // 0
	{85, 10, 15, 25, 30, 20, 0}, // 1
	{86, 10, 21, 29, 35, 25, 0}, // 2
	{87, 15, 27, 33, 40, 30, 0}, // 3
	{88, 15, 33, 37, 45, 35, 20}, // 4
	{90, 20, 40, 42, 50, 40, 25}, // 5
	{92, 20, 47, 47, 55, 45, 30}, // 6
	{94, 25, 55, 52, 60, 50, 35}, // 7
	{96, 25, 62, 57, 65, 55, 40}, // 8
	{98, 30, 70, 62, 70, 60, 45}, // 9
	{99, 30, 78, 67, 80, 65, 50}, // 10
	{99, 35, 86, 72, 90, 70, 55}, // 11
	{99, 35, 94, 77, 95, 75, 60}, // 12
	{99, 40, 99, 82, 99, 80, 65}, // 13
	{99, 40, 99, 87, 99, 85, 70}, // 14
	{99, 50, 99, 92, 99, 90, 75}, // 15
	{99, 50, 99, 97, 99, 95, 80}, // 16
	{99, 55, 99, 99, 99, 99, 80}, // 17
};

static int experience_paladin[21] = {
	0, 0, 2250, 4500, 9000, 18000, 36000, 75000, 150000, 300000,
	600000, 900000, 1200000, 1500000, 1800000, 2100000, 2400000, 2700000, 3000000, 3300000,
	3600000
};
static int experience_warrior[21] = {
	0, 0, 2000, 4000, 8000, 16000, 32000, 64000, 125000, 250000,
	500000, 750000, 1000000, 1250000, 1500000, 1750000, 2000000, 2250000, 2500000, 2750000,
	3000000
};
static int experience_wizard[21] = {
	0, 0, 2500, 5000, 10000, 20000, 40000, 60000, 90000, 135000,
	250000, 375000, 750000, 1125000, 1500000, 1875000, 2250000, 2625000, 3000000, 3375000, 3750000
};
static int experience_priest[21] = {
	0, 0, 1500, 3000, 6000, 13000, 27500, 55000, 110000, 225000,
	450000, 675000, 900000, 1125000, 1350000, 1575000, 1800000, 2025000, 2250000, 2475000, 2700000
};
static int experience_rogue[21] = {
	0, 0, 1250, 2500, 5000, 10000, 20000, 40000, 70000, 110000,
	160000, 220000, 440000, 660000, 880000, 1100000, 1320000, 1540000, 1760000, 1980000, 2200000
};

static int* get_experience_table(classn character_class) {
	switch(bsdata<classi>::elements[character_class].experience) {
	case 1: return experience_priest; // Cleric
	case 3: return experience_warrior; // Fighter
	case 4: return experience_paladin; // Paladin, Ranger
	case 2: return experience_wizard; // Mage
	default: return experience_rogue; // Rogue
	}
}

void creaturei::clear() {
	memset(this, 0, sizeof(*this));
	avatar = 0xFF;
	name = 0xFFFF;
	monster_id = 0xFFFF;
	x = -1;
	y = -1;
}

int	creaturei::getfood() const {
	return get(Constitution) * 100;
}

static void update_languages() {
	player->languages = player->getrace().languages;
	player->understand(player->race);
	if(player->getrace().origin)
		player->understand(player->getrace().origin);
	if(player->basic.abilities[Intellegence] >= 10)
		player->understand((racen)0); // All creatures with 11+ untellegence known common language
}

static void update_basic() {
	memcpy(player->abilities, player->basic.abilities, ExeptionalStrenght + 1);
	memcpy(player->feats, player->basic.feats, sizeof(player->basic.feats));
}

static int get_skill_level(featn v) {
	auto& ei = player->getclass();
	for(auto i = 0; i < ei.count; i++) {
		if(bsdata<classi>::elements[ei.classes[i]].is(v))
			return player->levels[i];
	}
	return 0;
}

static void update_basic_skills() {
	for(auto i = ClimbWalls; i <= ReadLanguages; i = (abilityn)(i + 1)) {
		auto level = imin(imax(0, get_skill_level(bsdata<abilityi>::elements[i].skill)), 17);
		auto value = theif_skill_basic[level][i - ClimbWalls];
		player->abilities[i] += value;
	}
}

static int get_maximum_hits() {
	auto n = player->getclass().count;
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

static void update_ability(abilityn v, int level, int per_level, int minimal) {
	if(level <= 0 || !per_level)
		return;
	auto value = player->abilities[v];
	if(value <= minimal)
		return;
	value -= level / per_level;
	if(value < minimal)
		value = minimal;
	player->abilities[v] = value;
}

static void update_abilities() {
	auto n = -(player->abilities[DrainStrenght] + player->abilities[DrainLevel]);
	auto s = -player->abilities[DrainLevel] * 5;
	player->add(Strenght, -player->abilities[DrainStrenght]);
	player->add(AttackMelee, n);
	player->add(AttackRange, n);
	player->add(Constitution, -player->abilities[DrainConstitution]);
	player->add(SaveVsParalization, s);
	player->add(SaveVsPoison, s);
	player->add(SaveVsTraps, s);
	player->add(SaveVsMagic, s);
	auto disease_level = player->abilities[DiseaseLevel];
	update_ability(Charisma, disease_level, 2, 6);
	update_ability(Dexterity, disease_level, 3, 6);
	update_ability(Strenght, disease_level, 5, 3);
}

static void update_theif_skill_by_dexterity() {
	auto a = player->get(Dexterity);
	auto p = maptbl(theif_skill_adjustment, a);
	player->add(PickPockets, p[0]);
	player->add(OpenLocks, p[1]);
	player->add(RemoveTraps, p[2]);
	player->add(MoveSilently, p[3]);
}

static void add_additional_spell(abilityn v) {
	if(player->abilities[v])
		player->abilities[v]++;
}

static void update_additional_spells() {
	auto c = player->getcaster();
	if(c != 0)
		return;
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
	if(k >= 19) {
		add_additional_spell(Spell1);
		add_additional_spell(Spell3);
	}
}

static void update_depended_abilities() {
	auto k = get_modified_strenght();
	player->abilities[AttackMelee] += maptbl(hit_probability, k);
	player->abilities[AttackRange] += maptbl(reaction_adjustment, player->abilities[Dexterity]);
	player->abilities[DamageMelee] += maptbl(damage_adjustment, k);
	player->abilities[AC] += maptbl(defence_adjustment, player->abilities[Dexterity]);
	player->abilities[ReactionBonus] += maptbl(cha_reaction_adjustment, player->abilities[Charisma]);
	player->abilities[LearnSpell] += maptbl(chance_learn_spell, player->abilities[Intellegence]);
	if(player->wears[RightHand])
		player->abilities[Speed] -= player->wears[RightHand].geti().speed;
	else
		player->abilities[Speed] -= player->wears[LeftHand].geti().speed;
	if(player->is(FeelPain))
		player->add(AttackMelee, -4);
	if(player->is(Blinded)) {
		player->add(AttackMelee, -4);
		player->add(AttackRange, -4);
	}
}

static void update_bonus_saves() {
	auto k = player->get(Constitution);
	if(player->is(BonusSaveVsPoison))
		player->abilities[SaveVsPoison] += maptbl(dwarven_bonus, k) * 5;
	if(player->is(BonusSaveVsSpells))
		player->abilities[SaveVsMagic] += maptbl(dwarven_bonus, k) * 5;
}

static int magic_wear_value(int magic_bonus) {
	return 17 + magic_bonus;
}

static void magic_wear(variant v) {
	if(v.iskind<abilityi>()) {
		auto m = bsdata<abilityi>::elements[v.value].wearing_multiplier;
		if(m == 100) {
			auto k = magic_wear_value(v.counter);
			auto n = player->abilities[v.value];
			if(n < k)
				n = k;
			player->abilities[v.value] = n;
		} else
			player->add((abilityn)v.value, v.counter * m);
	} else if(v.iskind<spelli>()) {
		// Nothing to do. Camp spell add.
	} else
		script_run(v);
}

static void update_wear() {
	auto push_modifier = modifier;
	modifier = Standart;
	for(auto& e : player->equipment()) {
		if(!e)
			continue;
		auto& ei = e.geti();
		if(ei.wear == LeftHand) {
			if(player->wears[RightHand] && player->wears[RightHand].is(TwoHanded))
				continue; // RULE: Two handed weapon
		}
		if(ei.wearing)
			script_run(ei.wearing);
		if((ei.wear >= Head && ei.wear <= Legs) // If wearable equipment only!
			&& ei.wear != LeftHand && ei.wear != RightHand) {
			auto power = e.getpower();
			if(power)
				magic_wear(power);
		}
	}
	modifier = push_modifier;
}

static void apply_boost(short type, short param) {
	if(type == BoostSpell)
		ftscript<spelli>(param, 0);
	else
		player->add((abilityn)type, param);
}

static void update_duration() {
	auto push_modifier = modifier; modifier = Wearing;
	referencei target = player;
	for(auto& e : bsdata<boosti>()) {
		if(e.target == target)
			apply_boost(e.type, e.param);
	}
	modifier = push_modifier;
}

static bool have_boost_summon(const item& it) {
	referencei target = player;
	for(auto& e : bsdata<boosti>()) {
		if(e.target == target && e.type == BoostSpell) {
			if(it.is(bsdata<spelli>::elements[e.param].summon))
				return true;
		}
	}
	return false;
}

static void update_summon() {
	for(auto& it : player->wears) {
		if(it.is(SummonedItem) && !have_boost_summon(it)) {
			auto w = it.geti().wear;
			it.clear();
			if(w == RightHand)
				change_quick_item(player, RightHand);
		}
	}
}

static int get_thac0_value() {
	auto& ei = player->getclass();
	auto result = 0;
	for(auto i = 0; i < ei.count; i++) {
		auto pi = bsdata<classi>::elements + ei.classes[i];
		auto level = imin(imax(0, (int)player->levels[i]), 21);
		auto save_group = imin(imax(0, (int)pi->save_group), 3);
		auto value = thac0_advance[save_group][level];
		if(value > result)
			result = value;
	}
	return result;
}

static int get_save_value(int save_index_value) {
	auto& ei = player->getclass();
	auto result = 20;
	for(auto i = 0; i < ei.count; i++) {
		auto pi = bsdata<classi>::elements + ei.classes[i];
		auto level = imin(imax(0, (int)player->levels[i]), 21);
		auto save_group = imin(imax(0, (int)pi->save_group), 3);
		auto value = saves_advance[save_group][save_index_value][level];
		if(value < result)
			result = value;
	}
	return (20 - result + 1) * 5;
}

static void update_saves() {
	for(auto i = SaveVsParalization; i <= SaveVsMagic; i = (abilityn)(i + 1))
		player->abilities[i] += get_save_value(save_index[i - SaveVsParalization]);
}

static void update_thac0() {
	auto value = get_thac0_value();
	player->abilities[AttackMelee] += value;
	player->abilities[AttackRange] += value;
}

static void update_bonus_experience() {
	auto& ei = player->getclass();
	if(player->get(ei.primary) >= 16)
		player->add(BonusExperience, 1);
}

void update_player() {
	update_basic();
	update_languages();
	update_summon();
	update_wear();
	update_duration();
	update_saves();
	update_thac0();
	update_basic_skills();
	update_abilities();
	update_depended_abilities();
	update_additional_spells();
	update_theif_skill_by_dexterity();
	update_bonus_saves();
	update_bonus_experience();
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

void generate_abilities() {
	auto& pc = player->getclass();
	auto& pr = player->getrace();
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
	iswap(player->basic.abilities[get_best_index(player->basic.abilities + Strenght, 6)], player->basic.abilities[pc.primary]);
	apply_minimal(player->basic.abilities, pc.minimal);
	apply_minimal(player->basic.abilities, pr.minimal);
	apply_maximal(player->basic.abilities, pr.maximal);
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

void roll_player_hits() {
	auto& ei = player->getclass();
	for(char i = 0; i < ei.count; i++) {
		auto pc = bsdata<classi>::elements + ei.classes[i];
		if(!pc->hd)
			continue;
		auto value = 1 + rand() % pc->hd;
		if(value < pc->hd / 2)
			value = pc->hd / 2;
		player->hpr += value;
	}
}

static void apply_class_ability() {
	auto& ei = player->getclass();
	for(char i = 0; i < ei.count; i++) {
		auto pd = bsdata<classi>::elements + ei.classes[i];
		player->levels[i]++;
		advance_level(pd, player->levels[i]);
	}
}

void apply_race_ability() {
	advance_level(bsdata<racei>::elements + player->race, 0);
}

void apply_player_script(const char* action) {
	auto& pc = player->getclass();
	auto& pr = player->getrace();
	auto p = bsdata<listi>::find(ids(pr.id, pc.id, action));
	if(!p)
		p = bsdata<listi>::find(ids(pc.id, action));
	if(p)
		script_run(p->elements);
}

static void set_starting_equipment() {
	apply_player_script("StartEquipment");
}

bool no_party_avatars(unsigned char value) {
	for(auto i = 0; i < 6; i++) {
		if(characters[i] && characters[i]->avatar == value)
			return false;
	}
	return true;
}

bool is_party_name(unsigned short value) {
	for(auto i = 0; i < 6; i++) {
		if(characters[i] && characters[i]->name == value)
			return true;
	}
	return false;
}

static void set_basic_ability() {
	player->basic.abilities[Alertness] += 70;
}

void update_player_hits() {
	player->hp = player->hpm;
	player->food = player->getfood();
}

void create_player_finish() {
	set_basic_ability();
	apply_class_ability();
	update_player();
	update_player_hits();
	set_starting_equipment();
	update_player();
	update_player_hits();
}

void create_player() {
	player = bsdata<creaturei>::addz();
	player->clear();
	create_npc(player, no_party_avatars, is_party_name);
	generate_abilities();
	apply_race_ability();
	roll_player_hits();
	create_player_finish();
}

static void apply_feats(const variants & elements) {
	auto push_modifier = modifier;
	modifier = Permanent;
	script_run(elements);
	modifier = push_modifier;
}

static void raise_monster_level() {
	if(player->levels[0]) {
		for(auto i = 1; i <= player->levels[0]; i++) {
			advance_level(&player->getclass(), i);
			player->hpr += xrand(1, bsdata<classi>::elements[player->character_class].hd);
		}
	} else
		player->hpr = xrand(1, 6);
}

static void apply_default_ability() {
	for(auto i = Strenght; i <= Charisma; i = (abilityn)(i + 1))
		player->basic.abilities[i] = 10;
	player->basic.set(UseMartial);
	player->basic.set(UseMetal);
	player->basic.set(UseLeather);
	player->basic.set(UseShield);
	player->basic.set(UsePriest);
	player->basic.set(UseMage);
}

void set_monster_spells() {
	auto pm = player->getmonster();
	if(!pm)
		return;
	memcpy(player->spells, pm->spells, sizeof(player->spells));
}

void create_monster(const monsteri * pi) {
	if(!pi)
		return;
	player->clear();
	player->monster_id = getbsi(pi);
	player->levels[0] = pi->hd;
	player->basic.abilities[AC] = (10 - pi->ac);
	player->alignment = pi->alignment;
	player->reaction = pi->reaction;
	set_basic_ability();
	apply_race_ability();
	apply_default_ability();
	update_basic();
	apply_feats(pi->feats);
	raise_monster_level();
	create_monster_pallette();
	set_monster_spells();
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
		if(p >= pi->wears && p <= pi->wears + LastInvertory)
			return (wearn)((item*)p - pi->wears);
	}
	return Backpack;
}

bool creaturei::isallow(const item & it) const {
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

void creaturei::additem(item & it) {
	if(isallow(it))
		equip(it);
	if(it)
		wearable::additem(it);
}

dice creaturei::getdamage(int& bonus, wearn id, bool large_enemy) const {
	auto& ei = wears[id].geti();
	auto power = wears[id].getpower();
	dice result = ei.damage;
	if(large_enemy && ei.damage_large)
		result = ei.damage_large;
	auto isranged = wears[id].isranged();
	bonus += player->get(isranged ? AttackRange : AttackMelee);
	result.b += player->get(isranged ? DamageRange : DamageMelee);
	// Only single player fighter have bonus speñialization (bonus attack keep)
	if(is(WeaponSpecialist) && isspecialist(&ei) && getclass().count == 1) {
		if(wears[id].isranged())
			bonus += 2;
		else {
			bonus += 1;
			result.b += 2;
		}
	}
	if(is(BonusVsElfWeapon) && wears[id].is(UseElvish))
		bonus += 1;
	result.b += power.counter;
	bonus += power.counter;
	return result;
}

int	creaturei::getchance(abilityn v) const {
	if(v >= Strenght && v <= Charisma)
		return abilities[v] * 5;
	else if(v >= SaveVsParalization && v <= DamageRange)
		return abilities[v];
	return 0;
}

bool roll_ability(int chance) {
	if(chance <= 0)
		return false;
	if(chance > 95)
		chance = 95;
	return d100() < chance;
}

bool creaturei::roll(abilityn v, int bonus) const {
	return roll_ability(getchance(v) + bonus);
}

const char* creaturei::getbadstate() const {
	if(isdead())
		return "Dead";
	else if(isdisabled())
		return "Disabled";
	else if(is(Paralized))
		return "Paralized";
	else if(is(PoisonLevel))
		return "Poisoned";
	else if(is(DiseaseLevel))
		return "Diseased";
	return 0;
}

void creaturei::addexp(int value) {
	if(abilities[BonusExperience])
		value += value * abilities[BonusExperience] / 10;
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

bool can_remove(item * pi, bool speech) {
	if(pi->is(SummonedItem))
		return false;
	if(pi->is(NaturalItem))
		return false;
	auto player = item_owner(pi);
	if(!player)
		return true;
	auto w = item_wear(pi);
	if(w >= Head && w <= Quiver) {
		if(pi->iscursed()) {
			if(speech) {
				player->say(speech_get("CantDropCursedItem"));
				pi->identify(1);
			}
			return false;
		}
	}
	return true;
}

bool can_loose(item * pi, bool speech) {
	if(pi->is(QuestItem)) {
		if(speech)
			player->say(speech_get("CantDropQuestItem"));
		return false;
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

bool creaturei::isready() const {
	if(isdisabled())
		return false;
	if(is(Paralized))
		return false;
	return true;
}

void creaturei::damage(damagen type, int value, char magic_bonus) {
	if(value <= 0)
		return;
	auto& ei = bsdata<damagei>::elements[type];
	if(ei.immunity && is(ei.immunity) && (magic_bonus <= 1))
		value = 0;
	else if(ei.resist && is(ei.resist))
		value = value / 2;
	switch(type) {
	case Poison:
		consolen(getnm(ids("Feel", bsdata<damagei>::elements[type].id)));
		break;
	default:
		fix_damage(this, value);
		break;
	}
	if(hp_aid > 0) {
		if(hp_aid >= value) {
			hp_aid -= value;
			value = 0;
		} else {
			value -= hp_aid;
			hp_aid = 0;
		}
	}
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
	if(is(ParalizeAttack))
		r += 1;
	if(basic.abilities[ResistMagic] >= 50)
		r += 1;
	if(basic.abilities[ResistMagic] >= 90)
		r += 1;
	auto exp = maptbl(hd_experience, r);
	if(r > 13)
		exp += (r - 13) * 1000;
	return exp;
}

void drop_unique_loot(creaturei * player) {
	for(auto& it : player->wears) {
		if(!it || it.is(NaturalItem) || it.is(SummonedItem))
			continue;
		if(it.is(Unique)) {
			it.identify(0);
			loc->drop(*player, it, get_side(player->side, party.d));
		}
	}
}

static void drop_loot(creaturei * player) {
	for(auto& it : player->wears) {
		if(!it || it.is(NaturalItem) || it.is(SummonedItem))
			continue;
		if(d100() < 15) {
			it.identify(0);
			loc->drop(*player, it, get_side(player->side, party.d));
		}
	}
}

void creaturei::kill() {
	if(!ismonster())
		return;
	party_addexp(getexpaward());
	party_addexp_per_killed(getlevel());
	if(loc) {
		drop_unique_loot(this);
		drop_loot(this);
		loc->state.monsters_killed++;
	}
	clear();
}

static bool isf(const creaturei * player, const item & weapon, featn v) {
	if(player->is(v) || weapon.geti().is(v))
		return true;
	return false;
}

bool creaturei::is(const item & weapon, featn v) const {
	if(is(v) || weapon.geti().is(v))
		return true;
	auto power = weapon.getpower();
	if(power.iskind<feati>() && power.value == v)
		return true;
	return false;
}

void creaturei::heal(int v) {
	if(v <= 0)
		return;
	v += hp;
	if(v >= hpm)
		v = hpm;
	hp = v;
}

void creaturei::add(abilityn i, int v) {
	if(v >= 0) {
		switch(i) {
		case DiseaseLevel:
			if(is(ImmuneDisease))
				return;
			break;
		case DrainStrenght:
			if(is(ProtectedFromEvil))
				return;
			break;
		default:
			break;
		}
	}
	statable::add(i, v);
}

void check_levelup() {
	auto& ei = player->getclass();
	auto experience = player->experience / ei.count;
	auto need_update = false;
	auto levelup_occurs = false;
	do {
		levelup_occurs = false;
		for(auto i = 0; i < ei.count; i++) {
			auto pi = bsdata<classi>::elements + ei.classes[i];
			auto tbl = get_experience_table(ei.classes[i]);
			auto lev = player->levels[i];
			if(experience >= tbl[lev + 1]) {
				player->levels[i]++;
				player->hpr += 1 + (rand() % pi->hd);
				advance_level(pi, player->levels[i]);
				consolen("%Name become %1i level %2", player->levels[i], pi->getname());
				need_update = true;
				levelup_occurs = true;
			}
		}
	} while(levelup_occurs);
	if(need_update)
		update_player();
}

int	creaturei::getcaster() const {
	return getclass().caster;
}

void set_reaction(creaturei * *creatures, reactions v) {
	for(auto i = 0; i < 6; i++) {
		if(creatures[i])
			creatures[i]->reaction = v;
	}
}

creaturei* get_leader(creaturei * *creatures) {
	creaturei* result = 0;
	for(auto i = 0; i < 6; i++) {
		if(!creatures[i] || creatures[i]->isdisabled())
			continue;
		if(!result)
			result = creatures[i];
		else if(result->getlevel() < creatures[i]->getlevel())
			result = creatures[i];
	}
	return result;
}

reactions get_reaction(creaturei * *creatures) {
	for(auto i = 0; i < 6; i++) {
		if(creatures[i] && creatures[i]->reaction != Indifferent)
			return creatures[i]->reaction;
	}
	return Indifferent;
}

static reactions* encounter_table(alignmentn monster_alignment) {
	static reactions hostile_players[encounter_table_maximum] = {
		Friendly, Friendly,
		Careful, Careful, Careful, Careful, Careful, Careful,
		Hostile, Hostile, Hostile, Hostile, Hostile, Hostile, Hostile, Hostile, Hostile, Hostile, Hostile,
	};
	static reactions indifferent_players[encounter_table_maximum] = {
		Friendly, Friendly, Friendly, Friendly, Friendly,
		Careful, Careful, Careful, Careful, Careful, Careful, Careful, Careful, Careful,
		Hostile, Hostile, Hostile, Hostile, Hostile,
	};
	static reactions friendly_players[encounter_table_maximum] = {
		Friendly, Friendly, Friendly, Friendly, Friendly, Friendly,
		Careful, Careful, Careful, Careful, Careful, Careful, Careful, Careful, Careful, Careful,
		Hostile, Hostile, Hostile,
	};
	switch(monster_alignment) {
	case LawfulGood:
	case NeutralGood:
		return friendly_players;
	case ChaoticGood:
	case LawfulNeutral:
	case TrueNeutral:
		return indifferent_players;
	default:
		return hostile_players;
	}
}

static reactions roll_reaction(alignmentn monster_alignment, int bonus) {
	auto n = (rand() % encounter_table_maximum) - bonus;
	auto t = encounter_table(monster_alignment);
	if(ismorale(monster_alignment, Lawful))
		bonus += 1;
	else if(ismorale(monster_alignment, Chaotic))
		bonus -= 1;
	if(n < 0)
		n = 0;
	else if(n > (encounter_table_maximum - 1))
		n = encounter_table_maximum - 1;
	return t[n];
}

void check_reaction(creaturei * *creatures, int bonus) {
	auto v = get_reaction(creatures);
	if(v == Indifferent) {
		auto charisma = party_median(characters, Charisma) + bonus;
		auto alignment = LawfulEvil;
		auto leader = get_leader(creatures);
		if(leader)
			alignment = leader->alignment;
		v = roll_reaction(alignment, maptbl(cha_reaction_adjustment, charisma));
		if(v == Indifferent)
			v = Careful;
		set_reaction(creatures, v);
	}
}

void change_quick_item(creaturei * player, wearn w) {
	if(!player || w != RightHand)
		return;
	if(!player->wears[FirstBelt]) {
		player->speak("NoQuickItem", 0);
		return;
	}
	if(!player->wears[FirstBelt].isallow(w))
		return;
	if(!player->isallow(player->wears[FirstBelt]))
		return;
	if(!can_remove(player->wears + w, true))
		return;
	auto it = player->wears[w];
	player->wears[w] = player->wears[FirstBelt];
	memmove(player->wears + FirstBelt, player->wears + SecondBelt, sizeof(it) * 2);
	player->wears[LastBelt].clear();
	if(it) {
		for(auto i = FirstBelt; i <= LastBelt; i = (wearn)(i + 1)) {
			if(!player->wears[i]) {
				player->wears[i] = it;
				break;
			}
		}
	}
}