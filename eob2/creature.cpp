#include "avatar.h"
#include "bsdata.h"
#include "class.h"
#include "creature.h"
#include "math.h"
#include "pushvalue.h"
#include "race.h"
#include "rand.h"
#include "slice.h"
#include "unit.h"

BSDATAC(creaturei, 256)

creaturei* player;

static char hit_points_adjustment[] = {
	-4,
	-3, -2, -2, -1, -1, -1, 0, 0, 0,
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
}

static void update_basic() {
	memcpy(player->abilities, player->basic.abilities, sizeof(player->abilities));
}

static int get_maximum_hits() {
	auto m = player->getlevel();
	auto a = player->get(Constitution);
	auto r = player->get(Hits) + maptbl(hit_points_adjustment, a) * m;
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

static void all_saves(int v) {
	player->abilities[SaveVsParalization] += v;
	player->abilities[SaveVsPoison] += v;
	player->abilities[SaveVsTraps] += v;
	player->abilities[SaveVsMagic] += v;
}

static void update_combat_stats() {
	auto k = get_modified_strenght();
	player->abilities[AttackMelee] += maptbl(hit_probability, k);
	player->abilities[AttackRange] += maptbl(reaction_adjustment, player->abilities[Dexterity]);
	player->abilities[DamageMelee] += maptbl(damage_adjustment, k);
	player->abilities[AC] += maptbl(defence_adjustment, player->abilities[Dexterity]);
}

static void update_bonus_saves() {
	auto k = player->get(Constitution);
	if(player->is(BonusSaveVsPoison))
		player->abilities[SaveVsPoison] += maptbl(dwarven_bonus, k) * 5;
	if(player->is(BonusSaveVsSpells))
		player->abilities[SaveVsMagic] += maptbl(dwarven_bonus, k) * 5;
}

static void update_wear() {
}

static void update_duration() {
}

void update_player() {
	update_basic();
	update_wear();
	update_duration();
	update_combat_stats();
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

static void generate_abilities() {
	char result[12] = {};
	if(false) {
		for(size_t i = 0; i < sizeof(result) / sizeof(result[0]); i++)
			result[i] = (rand() % 6) + (rand() % 6) + (rand() % 6) + 3;
		qsort(result, sizeof(result) / sizeof(result[0]), sizeof(result[0]), compare_char_desc);
		zshuffle(result, 6);
	} else {
		for(size_t i = 0; i < 6; i++)
			result[i] = get_best_4d6();
	}
	for(size_t i = 0; i < 6; i++)
		player->abilities[Strenght + i] = result[i];
	auto pc = bsdata<classi>::elements + player->type;
	iswap(player->abilities[get_best_index(player->abilities + Strenght, 6)], player->abilities[pc->primary]);
	player->abilities[ExeptionalStrenght] = d100() + 1;
}

static size_t party_avatars(unsigned char* result) {
	auto ps = result;
	for(auto p : party.units) {
		if(p->avatar != 0xFF)
			*ps++ = p->avatar;
	}
	return ps - result;
}

static void generate_avatar() {
	unsigned char exclude[6];
	auto size = party_avatars(exclude);
	player->avatar = get_avatar(player->race, player->gender, player->type, exclude, size);
}

void create_player(const racei* pr, gendern gender, const classi* pc) {
	if(!pr || !pc)
		return;
	player = bsdata<creaturei>::add();
	player->clear();
	player->race = (racen)bsdata<racei>::source.indexof(pr);
	player->gender = gender;
	player->type = (classn)bsdata<classi>::source.indexof(pc);
	generate_abilities();
	player->avatar = get_avatar(player->race, gender, player->type);
	update_player();
}