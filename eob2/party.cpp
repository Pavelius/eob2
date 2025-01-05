#include "boost.h"
#include "bsdata.h"
#include "cell.h"
#include "class.h"
#include "console.h"
#include "creature.h"
#include "direction.h"
#include "draw.h"
#include "dungeon.h"
#include "list.h"
#include "math.h"
#include "party.h"
#include "rand.h"
#include "script.h"
#include "shop.h"
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

void restore_spells(int bonus);

static void update_default_spells() {
	auto& ei = player->getclass();
	auto spell_known = get_spells_known(player);
	if(!spell_known)
		return;
	auto spell_prepared = get_spells_prepared(player);
	if(!spell_prepared)
		return;
	for(auto i = 0; i < ei.count; i++) {
		auto pc = bsdata<classi>::elements + ei.classes[i];
		if(pc->caster == -1)
			continue;
		auto pi = bsdata<listi>::find(str("Default%1Spells", pc->id));
		if(!pi)
			continue;
		for(auto level = 1; level < 9; level++) {
			auto slot_left = player->get((abilityn)(level + Spell1 - 1));
			for(auto v : pi->elements) {
				if(slot_left <= 0)
					break;
				if(v.iskind<spelli>()) {
					if(!spell_known->is(v.value))
						continue;
					auto spell_level = bsdata<spelli>::elements[v.value].levels[pc->caster];
					if(spell_level != level)
						continue;
					auto n = slot_left;
					auto k = v.counter;
					if(k == 0)
						k = 1;
					if(n > k)
						n = k;
					spell_prepared[v.value] += n;
					slot_left -= n;
				}
			}
		}
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
	update_default_spells();
	restore_spells(0);
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

void party_addexp(alignmentn v, int value) {
	for(auto p : characters) {
		if(p && !p->isdisabled() && p->is(v))
			p->addexp(value);
	}
}

void party_addexp_per_killed(int hd) {
	for(auto p : characters) {
		if(!p || p->isdisabled())
			continue;
		p->addexp(hd * p->getclass().exp_per_hd / 100);
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
		reaction_check(0);
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
		else if(e.roll(Dexterity)) {
			auto d = random_free_look(e, e.d);
			if(d != Center)
				monster_move(e, d);
		} else
			monsters_stop(e);
	}
}

static void check_regeneration() {
	if(player->is(Regenerated))
		player->heal(1);
}

static void check_poison() {
	if(player->is(StoppedPoison))
		return;
	if(!player->is(PoisonLevel))
		return;
	auto penalty = player->get(PoisonLevel) / 5;
	if(!player->roll(SaveVsPoison, -penalty))
		player->damage(Poison, 1);
	player->add(PoisonLevel, -1);
}

static void check_acid() {
	auto damage = 0;
	if(player->is(AcidD1Level)) {
		damage += xrand(1, 4);
		player->add(AcidD1Level, -1);
	}
	if(player->is(AcidD2Level)) {
		damage += xrand(1, 4);
		player->add(AcidD2Level, -1);
	}
	if(damage)
		player->damage(Acid, damage);
}

static void check_disease() {
	if(!player->is(DiseaseLevel))
		return;
	// Two test in row to overcome disease or two failed tests to get worse
	if(player->roll(SaveVsPoison, 0)) {
		if(player->roll(SaveVsPoison, 0))
			player->add(DiseaseLevel, -1);
	} else {
		if(!player->roll(SaveVsPoison, 0)) {
			player->add(DiseaseLevel, 1);
			consolen(getnm("FeelDisease"));
		}
	}
	// Reduce hp (can die if disease level high)
	if(player->is(DiseaseLevel)) {
		auto m = player->hpm / 3;
		if(player->get(DiseaseLevel) > 10)
			m = 0;
		if(player->hp > m)
			player->hp--;
	}
}

static void random_characters() {
	memcpy(monsters, characters, sizeof(monsters));
	zshuffle(monsters, 6);
}

static size_t shrink(creaturei** target, creaturei** source) {
	auto ps = target;
	for(auto i = 0; i < 6; i++) {
		if(source[i] && !source[i]->isdisabled())
			*ps++ = source[i];
	}
	return ps - target;
}

static slice<creaturei*> random_party() {
	static creaturei* monsters[6];
	auto count = shrink(monsters, characters);
	zshuffle(monsters, count);
	return slice<creaturei*>(monsters, count);
}

static bool check_secrets(directions d) {
	if(!loc)
		return false;
	auto po = loc->get(party, to(party.d, d));
	if(!po || po->type != CellSecretButton)
		return false;
	for(auto p : random_party()) {
		if(!p->roll(DetectSecrets, -10))
			continue;
		p->speak("DetectSecrets", "Success");
		return true;
	}
	return false;
}

static void check_secrets() {
	if(check_secrets(Right))
		return;
	if(check_secrets(Left))
		return;
}

static bool check_noises_behind_door(directions d) {
	if(!loc)
		return false;
	auto v = to(party, to(party.d, d));
	auto t = loc->get(v);
	if(t != CellDoor || loc->is(v, CellActive) || loc->is(v, CellExperience))
		return false;
	creaturei* monsters[6]; loc->getmonsters(monsters, to(v, to(party.d, d)));
	auto count = shrink(monsters, monsters);
	loc->set(v, CellExperience);
	for(auto p : random_party()) {
		if(!p->roll(HearNoise))
			continue;
		p->addexp(20);
		if(count) {
			if(count == 1 && monsters[0]->is(Large))
				p->speak("HearNoise", "Large");
			else
				p->speak("HearNoise", "Medium", count);
		} else
			p->speak("HearNoise", "Nobody");
		return true;
	}
	return true;
}

static void check_noises_behind_door() {
	if(check_noises_behind_door(Right))
		return;
	if(check_noises_behind_door(Left))
		return;
	if(check_noises_behind_door(Up))
		return;
}

static void check_shops() {
	for(auto& e : bsdata<shopi>())
		e.update(party.abilities[Minutes]);
}

static void update_every_round() {
	player->remove(Moved);
	update_player();
	check_regeneration();
	check_acid();
}

static void update_every_turn() {
	check_poison();
}

static void update_every_hour() {
	check_disease();
	check_shops();
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

static void group_damage(creaturei** creatures, damagen type, dice damage) {
	for(auto i = 0; i < 6; i++) {
		auto p = creatures[i];
		if(!p)
			continue;
		p->damage(type, damage.roll());
	}
}

static void trap_launch(pointc v, directions d, damagen type, dice damage) {
	auto start = v;
	while(v) {
		if(party == v) {
			if(to(party.d, Down) == d && party.x == v.x || party.y == v.y)
				thrown_item(start, Down, 2, -1, start.distance(party) + 1);
			group_damage(characters, type, damage);
			break;
		} else if(loc->ismonster(v)) {
			creaturei* creatures[6]; loc->getmonsters(creatures, v);
			group_damage(creatures, type, damage);
			break;
		} else {
			auto t = loc->get(v);
			if(t == CellDoor && loc->is(v, CellActive))
				break;
			else if(t == CellWall || t == CellStairsUp || t == CellStairsDown)
				break;
		}
		v = to(v, d);
	}
}

static void update_floor_state() {
	if(!loc)
		return;
	unsigned char map[mpy][mpx] = {0};
	loc->state.monsters_alive = 0;
	loc->state.items_lying = 0;
	loc->state.explored_passable = loc->getpassables(true);
	if(party)
		map[party.y][party.x]++;
	for(auto& e : loc->monsters) {
		if(!e)
			continue;
		loc->state.monsters_alive++;
		if(map[e.y][e.x] > 0)
			continue;
		map[e.y][e.x]++;
	}
	for(auto& e : loc->items) {
		if(!e)
			continue;
		loc->state.items_lying++;
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
							if(!po->is(CellActive))
								trap_launch(*po, to(po->d, Down), Fire, {1, 6});
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

static void check_all_goals() {
	if(!loc)
		return;
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

static void party_clear() {
	memset(characters, 0, sizeof(characters));
	memset(spells_prepared, 0, sizeof(spells_prepared));
	bsdata<creaturei>::source.clear();
	bsdata<dungeoni>::source.clear();
	party.clear();
}

int party_goal(unsigned short quest_id, goaln v) {
	auto result = 0;
	for(auto& e : bsdata<dungeoni>()) {
		if(e.quest_id == quest_id && e.is(v))
			result++;
	}
	return result;
}

static void check_goals() {
	if(!loc || !player)
		return;
	for(auto i = (goaln)0; i <= KillAlmostAllMonsters; i = (goaln)(i + 1)) {
		if(loc->rewards.is(i))
			continue;
		auto& ei = bsdata<goali>::elements[i];
		if(ei.test()) {
			loc->rewards.set(i);
			if(!player->isdisabled())
				player->speak(ei.id, "Reward");
			party_addexp(ei.experience);
		}
	}
}

static void clear_boost_proc(referencei target, variant v) {
	if(v.iskind<spelli>()) {
		auto pi = bsdata<spelli>::elements + v.value;
		if(pi->clearing) {
			auto push = player;
			player = target;
			script_run(pi->clearing);
			player = push;
		}
	}
}

static void check_food() {
	if(player->food > 0)
		player->food--;
	else if(d100() < 3)
		player->speakn("Need", "Rest");
}

void pass_round() {
	clear_boost(party.abilities[Minutes], clear_boost_proc);
	monsters_movement();
	add_party(Minutes, 1);
	animation_update();
	update_floor_state();
	check_secrets();
	check_noises_behind_door();
	all_creatures(update_every_round);
	all_party(check_levelup, true);
	all_party(check_food, true);
	if((party.abilities[Minutes] % 6) == 0)
		all_creatures(update_every_turn);
	if((party.abilities[Minutes] % 60) == 0)
		all_creatures(update_every_hour);
	check_goals();
	fix_animate();
	if(all_party_disabled()) {
		message_box(getnm("AllPartyDead"));
		party_clear();
		set_next_scene(main_menu);
	}
}

void pass_hours(int value) {
	animation_update();
	add_party(Minutes, 60 * value);
	clear_boost(party.abilities[Minutes], clear_boost_proc);
	all_creatures(update_every_round);
	for(auto i = 0; i < 6 * value; i++)
		all_creatures(update_every_turn);
	for(auto i = 0; i < value; i++)
		all_creatures(update_every_hour);
	all_party(check_levelup, true);
	fix_animate();
}

int party_best(creaturei** creatures, abilityn v, bool set_player) {
	auto bi = -1;
	auto bv = -1;
	for(int i = 0; i < 6; i++) {
		if(!creatures[i] || !creatures[i]->isready())
			continue;
		if(bv > creatures[i]->get(v))
			continue;
		bv = creatures[i]->get(v);
		bi = i;
	}
	if(bi != -1 && set_player)
		player = characters[bi];
	return bv;
}

int party_median(creaturei** creatures, abilityn v) {
	auto count = 0;
	auto value = 0;
	for(int i = 0; i < 6; i++) {
		if(!creatures[i] || !creatures[i]->isready())
			continue;
		value += creatures[i]->abilities[v];
		count++;
	}
	if(!count)
		return 0;
	return value / count;
}


bool party_roll(abilityn v, int bonus) {
	auto chance = party_median(characters, v);
	if(v >= Strenght && v <= Charisma)
		chance *= 5;
	if(chance <= 0)
		return false;
	chance += bonus * 5;
	return roll_ability(chance);
}

quest* partyi::getquest() const {
	return getbs<quest>(quest_id);
}

locationi* partyi::getlocation() const {
	return getbs<locationi>(location_id);
}

int	partyi::getstage(const quest* v) const {
	auto n = getbsi(v);
	if(n == 0xFFFF)
		return 0;
	return stages[n];
}

void partyi::clear() {
	memset(this, 0, sizeof(*this));
	posable::clear();
}

item* party_get_item(const itemi* pi) {
	for(auto p : characters) {
		if(!p)
			continue;
		for(auto& it : p->wears) {
			if(it && it.is(pi))
				return &it;
		}
	}
	return 0;
}

unsigned get_stamp(unsigned duration) {
	return party.abilities[Minutes] + duration;
}

bool party_is(alignmentn v) {
	for(auto i = 0; i < 6; i++) {
		if(characters[i] && !characters[i]->isdisabled() && characters[i]->is(v))
			return true;
	}
	return false;
}

bool party_is(creaturei* player) {
	for(auto p : characters) {
		if(p == player)
			return true;
	}
	return false;
}

bool party_is(featn v) {
	for(auto i = 0; i < 6; i++) {
		if(characters[i] && !characters[i]->isdisabled() && characters[i]->is(v))
			return true;
	}
	return false;
}

static bool if_take_special_item() {
	return loc->special && party_get_item(bsdata<itemi>::elements + loc->special);
}

static bool if_kill_almost_all_monsters() {
	return loc->state.monsters_killed >= (90 * loc->state.monsters / 100);
}

static bool if_kill_boss() {
	return loc->boss && loc->getmonster(loc->boss) == 0;
}

static bool if_kill_boss_minions() {
	return loc->minions && loc->getmonster(loc->minions) == 0;
}

static bool if_disable_all_traps() {
	return loc->state.wallmessages[MessageTraps] && loc->state.traps_disabled >= loc->state.wallmessages[MessageTraps];
}

static bool if_open_all_locked_doors() {
	return loc->state.wallmessages[MessageLocked] && loc->state.locks_open >= loc->state.wallmessages[MessageLocked];
}

static bool if_find_all_secrets() {
	return loc->state.wallmessages[MessageSecrets] && loc->state.secrets_found >= loc->state.wallmessages[MessageSecrets];
}

static bool if_explore_most_area() {
	return loc->state.explored_passable >= (90 * loc->state.total_passable / 100);
}

BSDATA(goali) = {
	{"ExmploreMostArea", if_explore_most_area, 500},
	{"FindAllSecrets", if_find_all_secrets, 1000},
	{"TakeSpecialItem", if_take_special_item, 500},
	{"OpenAllLockedDoors", if_open_all_locked_doors, 1000},
	{"DisableAllTraps", if_disable_all_traps, 1000},
	{"KillBoss", if_kill_boss, 800},
	{"KillBossMinions", if_kill_boss_minions, 800},
	{"KillAlmostAllMonsters", if_kill_almost_all_monsters, 800},
};
assert_enum(goali, KillAlmostAllMonsters)