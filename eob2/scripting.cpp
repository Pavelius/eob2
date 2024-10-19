#include "action.h"
#include "answers.h"
#include "cell.h"
#include "class.h"
#include "console.h"
#include "creature.h"
#include "direction.h"
#include "draw.h"
#include "dungeon.h"
#include "formula.h"
#include "gender.h"
#include "hotkey.h"
#include "list.h"
#include "location.h"
#include "modifier.h"
#include "monster.h"
#include "party.h"
#include "quest.h"
#include "race.h"
#include "rand.h"
#include "resid.h"
#include "script.h"
#include "speech.h"
#include "spell.h"
#include "textscript.h"
#include "view.h"
#include "view_focus.h"
#include "wallmessage.h"

extern "C" void exit(int code);

racei* last_race;
classi* last_class;
static void* last_result;
static adat<creaturei*, 16> combatants;

static int get_bonus(int v) {
	switch(v) {
	case 101: return last_number;
	case -101: return -last_number;
	default: return v;
	}
}

template<> void ftscript<racei>(int value, int bonus) {
	last_race = bsdata<racei>::elements + value;
}

template<> void ftscript<classi>(int value, int bonus) {
	last_class = bsdata<classi>::elements + value;
}

template<> void ftscript<formulai>(int value, int bonus) {
	auto p = bsdata<formulai>::elements + value;
	last_number = p->proc(last_number, bonus);
}

template<> void ftscript<genderi>(int value, int bonus) {
	last_gender = (gendern)value;
}

template<> void ftscript<locationi>(int value, int bonus) {
	last_location = bsdata<locationi>::elements + value;
}

template<> void ftscript<actioni>(int value, int bonus) {
	last_action = bsdata<actioni>::elements + value;
}

template<> void ftscript<quest>(int value, int bonus) {
	last_quest = bsdata<quest>::elements + value;
}

template<> void ftscript<feati>(int value, int bonus) {
	switch(modifier) {
	case Permanent:
		player->basic.setfeat((featn)value);
		break;
	default:
		player->setfeat((featn)value);
		break;
	}
}

template<> void ftscript<abilityi>(int value, int bonus) {
	last_ability = (abilityn)value;
	if(!bonus)
		return;
	switch(modifier) {
	case Permanent: add_value(player->basic.abilities[value], get_bonus(bonus)); break;
	default: add_value(player->abilities[value], get_bonus(bonus)); break;
	}
}

template<> void ftscript<partystati>(int value, int bonus) {
	add_party((partystatn)value, get_bonus(bonus));
}

template<> void ftscript<itemi>(int value, int bonus) {
	item v; v.create(value);
	player->additem(v);
}

static dungeoni* find_dungeon(int level) {
	auto id = getbsi(last_quest);
	if(id == 0xFFFF)
		return 0;
	for(auto& e : bsdata<dungeoni>()) {
		if(e.quest_id == id && e.level == level)
			return &e;
	}
	return 0;
}

static const char* get_action() {
	if(last_action)
		return last_action->id;
	else if(last_list)
		return last_list->id;
	else
		return "GlobalList";
}

static void attack_modify(int bonus) {
	ftscript<abilityi>(AttackMelee, bonus);
	ftscript<abilityi>(AttackRange, bonus);
}

static void damage_modify(int bonus) {
	ftscript<abilityi>(DamageMelee, bonus);
	ftscript<abilityi>(DamageRange, bonus);
}

static void saves_modify(int bonus) {
	bonus *= 5;
	ftscript<abilityi>(SaveVsMagic, bonus);
	ftscript<abilityi>(SaveVsParalization, bonus);
	ftscript<abilityi>(SaveVsPoison, bonus);
	ftscript<abilityi>(SaveVsTraps, bonus);
}

void pass_round() {
	add_party(Minutes, 1);
}

void skip_hours(int value) {
	add_party(Minutes, 60 * value);
}

static void turnto(pointc v, directions d, bool* surprise = 0) {
	if(!d)
		return;
	if(v == party) {
		if(surprise)
			*surprise = (party.d != d);
		set_party_position(v, d);
	} else {
		creaturei* result[4]; loc->getmonsters(result, v, party.d);
		for(auto pc : result) {
			if(!pc)
				continue;
			if(surprise && !(*surprise))
				*surprise = (pc->d != d);
			pc->d = d;
		}
	}
}

static void create_character(int bonus) {
	create_player(last_race, last_gender, last_class);
}

static const char* get_header(const char* id, const char* action) {
	auto pn = getnme(ids(id, action));
	if(!pn && action)
		pn = getnm(ids("Global", action));
	return pn;
}

static void add_menu(variant& v) {
	if(v.iskind<actioni>()) {
		auto p = bsdata<actioni>::elements + v.value;
		if(p->isallow(player))
			an.add(v.getpointer(), v.getname());
	} else if(v.iskind<formulai>()) {
		auto p = bsdata<formulai>::elements + v.value;
		an.add(&v, getnm(ids(p->id, get_action())), p->proc(last_number, v.counter));
	} else if(v.iskind<script>()) {
		auto p = bsdata<script>::elements + v.value;
		an.add(p, getnm(p->id));
	} else if(v.iskind<locationi>()) {
		auto format = get_header(v.getid(), "Visit");
		an.add(v.getpointer(), format, v.getname());
	} else
		an.add(v.getpointer(), v.getname());
}

static void enter_location(int bonus);

static void apply_action(int bonus) {
	if(last_action->avatar)
		picture = last_action->avatar;
	script_run(last_action->effect);
}

static void apply_result() {
	if(bsdata<locationi>::have(last_result)) {
		last_location = (locationi*)last_result;
		enter_location(0);
	} else if(bsdata<script>::have(last_result))
		((script*)last_result)->proc(0);
	else if(bsdata<listi>::have(last_result))
		ftscript<listi>(bsdata<listi>::source.indexof(last_result), 0);
	else if(bsdata<actioni>::have(last_result)) {
		last_action = (actioni*)last_result;
		apply_action(0);
	} else if(bsdata<variant>::have(last_result))
		script_run(*((variant*)last_result));
}

static void choose_options(const char* id, const variants& options) {
	// Function use huge amount of memory for existing copy of answers and return result to upper level.
	// So this memory used only for selection, not for each level of hierarhi.
	pushanswer push;
	char header[64]; stringbuilder sb(header);
	set_player_by_focus();
	sb.add(get_header(id, "Options"), getnm(id));
	for(auto& v : options)
		add_menu(v);
	last_result = choose_answer(header, getnm("Cancel"), paint_city_menu, button_label, 1);
}

static void choose_city_menu() {
	auto& e = bsdata<locationi>::elements[party.location];
	choose_options("Visit", e.options);
	apply_result();
}

static void choose_dungeon_menu() {
	auto pi = bsdata<listi>::find("CampMenu");
	if(!pi)
		return;
	choose_options(pi->id, pi->elements);
	apply_result();
}

static void drop_city_item() {
	auto pi = (item*)current_focus;
	auto pn = item_owner(pi);
	if(!pn)
		return;
	auto cost = pi->getcost() / 2;
	if(!cost)
		return;
	if(!can_remove(pi))
		return;
	char temp[128]; stringbuilder sb(temp);
	sb.add(getnm("SellConfirm"), pi->getname(), cost);
	if(!confirm(temp))
		return;
	pi->clear();
	add_party(GoldPiece, cost);
}

static void use_item() {
	auto pi = (item*)current_focus;
	auto pn = item_owner(pi);
	if(!pn)
		return;
	auto w = item_wear(pi);
	auto& ei = pi->geti();
	switch(ei.wear) {
	case LeftHand:
	case RightHand:
		if(w != LeftHand && w != RightHand)
			pn->speak("MustBeUseInHand", pi->getname());
		else if(pi->isweapon()) {
			// TODO: Make attack
		} else if(ei.use) {

		}
		break;
	case Body: case Neck: case Elbow: case Legs: case Head:
		pn->speak("MustBeWearing", pi->getname());
		break;
	default:
		break;
	}
}

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
	loc->getmonsters(combatants.data, position, Up);
	combatants.count = shrink_creatures(combatants.data, combatants.data, 4);
	if(!combatants)
		return false;
	combatants.count += shrink_creatures(combatants.data + combatants.count, characters, 6);
	for(auto p : combatants)
		p->initiative = xrand(1, 10) + p->get(Speed);
	qsort(combatants.data, combatants.count, sizeof(combatants.data[0]), compare_creatures);
	return true;
}

static creaturei* get_enemy(const creaturei* player) {
	static int sides[][6] = {
		{0, 1, 2, 3, 4, 5},
		{1, 0, 3, 2, 5, 4},
	};
	if(player->ismonster()) {
		auto side = get_side(player->side, party.d);
		for(auto pi : sides[side % 2]) {
			if(characters[pi] && !characters[pi]->isdisabled())
				return characters[pi];
		}
	} else {
		auto side = get_side(player->side, party.d);
		for(auto p : combatants) {
			if(p->isdisabled())
				continue;
			if(!p->ismonster())
				continue;
			return p;
		}
	}
	return 0;
}

static void make_attack(creaturei* player, creaturei* enemy, wearn wear, int bonus, int multiplier) {
}

static void make_full_attack(creaturei* player, creaturei* enemy, int bonus, int multiplier) {
	if(!enemy)
		return;
	auto wp1 = player->wears[RightHand];
	auto wp2 = player->wears[LeftHand];
	auto wp3 = player->wears[Head];
	if(wp1.is(TwoHanded) || !wp2.isweapon())
		wp2.clear();
	if(!wp3.isweapon())
		wp3.clear();
	if(wp2) {
		make_attack(player, enemy, RightHand, bonus + player->gethitpenalty(-4), multiplier);
		make_attack(player, enemy, LeftHand, bonus + player->gethitpenalty(-6), multiplier);
	} else
		make_attack(player, enemy, RightHand, bonus, multiplier);
	if(wp3)
		make_attack(player, enemy, Head, bonus, multiplier);
}

static void make_melee_round() {
	if(!select_combatants(to(party, party.d)))
		return;
	for(auto p : combatants) {
		make_full_attack(p, get_enemy(p), 0, 1);
		fix_animate();
	}
}

static void test_dungeon() {
	make_melee_round();
}

static void city_adventure_input() {
	static hotkeyi keys[] = {
		{KeyEscape, choose_city_menu},
		{'D', drop_city_item},
		{'U', use_item},
		{'T', test_dungeon},
		{}};
	city_input(keys);
}

static void* save_focus;

static void play_location() {
	show_scene(paint_city, city_adventure_input, save_focus);
}

static void enter_location(int bonus) {
	loc = 0;
	party.location = getbsi(last_location);
	picture = last_location->avatar;
	save_focus = current_focus;
	set_next_scene(play_location);
}

static void return_to_street(int bonus) {
	last_location = bsdata<locationi>::elements + party.location;
	if(last_location->parent) {
		last_location = last_location->parent;
		enter_location(0);
	}
}

static void confirm_action(int bonus) {
	const char* id = "AskConfirm";
	if(last_action)
		id = last_action->id;
	if(!confirm(getnme(ids(id, "Confirm"))))
		script_stop();
}

static void exit_game(int bonus) {
	if(confirm(getnme("ExitConfirm")))
		exit(0);
}

static void eat_and_drink(int bonus) {
}

static void save_game(int bonus) {
	save_game("autosave");
}

static void load_game(int bonus) {
	read_game("autosave");
}

static void choose_menu(int bonus) {
	variants commands; commands.set(script_begin, script_end - script_begin);
	choose_options(get_action(), commands);
	apply_result();
	script_stop();
}

void add_spells(int type, int level, const spellseta* include) {
	an.clear();
	for(auto& e : bsdata<spelli>()) {
		if(e.levels[type] != level)
			continue;
		auto index = getbsi(&e);
		if(include && !include->is(index))
			continue;
		an.add(&e, e.getname());
	}
}

static void activate_quest(int bonus) {
	if(!last_quest)
		return;
	if(bonus >= 0) {
		auto index = getbsi(last_quest);
		if(!party.prepared.is(index)) {
			party.prepared.set(index);
			dungeon_create();
		}
		party.active.set(index);
	} else
		party.active.remove(bsdata<quest>::source.indexof(last_quest));
}

static void done_quest(int bonus) {
	if(!last_quest)
		return;
	if(bonus >= 0)
		party.done.set(bsdata<quest>::source.indexof(last_quest));
	else
		party.done.remove(bsdata<quest>::source.indexof(last_quest));
}

static void choose_adventure() {
	auto push_answers = an;
	an.clear();
	for(auto& e : bsdata<quest>()) {
		auto index = getbsi(&e);
		if(!party.active.is(index))
			continue;
		if(party.done.is(index))
			continue;
		an.add(&e, e.getname());
	}
	an.sort();
	last_quest = (quest*)choose_answer(getnm("PartysAdventureAsk"), getnm("Cancel"), paint_city_menu, button_label, 1);
	an = push_answers;
}

static bool choose_dialog(const char* format, const char* format_param, const char* yes, const char* no) {
	auto push = an; an.clear();
	an.add((void*)1, yes);
	auto result = (bool)dialogv(no, format, format_param);
	an = push;
	return result;
}

static void message(const char* format, const char* format_param = 0) {
	auto push = an; an.clear();
	dialogv(0, format);
	an = push;
}

static bool is_passable(pointc v) {
	if(!v)
		return false;
	auto& ei = bsdata<celli>::elements[loc->get(v)];
	return ei.flags.is(Passable) || (ei.flags.is(PassableActivated) && loc->is(v, CellActive));
}

static void move_party_left() {
	move_party(to(party, to(party.d, Left)));
}

static void move_party_right() {
	move_party(to(party, to(party.d, Right)));
}

static void move_party_up() {
	move_party(to(party, party.d));
}

static void move_party_down() {
	move_party(to(party, to(party.d, Down)));
}

static void turn_right() {
	party.d = to(party.d, Right);
}

static void turn_left() {
	party.d = to(party.d, Left);
}

static void explore_area() {
	loc->set(party, CellExplored);
	loc->set(to(party, Up), CellExplored);
	loc->set(to(party, Down), CellExplored);
	loc->set(to(party, Left), CellExplored);
	loc->set(to(party, Right), CellExplored);
}

static void make_action() {
	explore_area();
}

static void activate(pointc v, bool value) {
	if(value)
		loc->set(v, CellActive);
	else
		loc->remove(v, CellActive);
}

static void toggle(pointc v) {
	if(!v)
		return;
	if(!loc->is(v, CellActive))
		loc->set(v, CellActive);
	else
		loc->remove(v, CellActive);
}

static bool change_cell(pointc v) {
	if(!v)
		return true;
	auto t = loc->get(v);
	auto n = bsdata<celli>::elements[t].activate;
	if(!n)
		return false;
	loc->set(v, n);
	return true;
}

static bool change_overlay(pointc v, directions d) {
	auto p = loc->get(v, d);
	if(!p)
		return false;
	auto x = to(v, d);
	if(!x)
		return false;
	auto n = bsdata<celli>::elements[p->type].activate;
	if(!n)
		return false;
	loc->set(x, n);
	loc->removeov(x);
	return true;
}

static void read_wall_messages(creaturei* player, dungeoni::overlayi* p) {
	auto p1 = getid<wallmessagei>(p->subtype);
	if(p->subtype < MessageHabbits) {
		if(loc->state.wallmessages[p->subtype] > 0)
			player->speak(p1, "Present");
		else
			player->speak(p1, "Miss");
	} else
		player->speak(p1, "Read");
}

static void manipulate() {
	auto v = to(party, party.d);
	auto player = item_owner(current_focus);
	if(!player)
		return;
	item* pi = (item*)current_focus;
	auto t = loc->get(v);
	if(t == CellPortal) {
		player->speak(getid<celli>(t), "About");
		make_action();
		return;
	}
	auto p = loc->get(party, party.d);
	if(!p)
		return;
	switch(p->type) {
	case CellDoorButton:
		toggle(v);
		break;
	case CellDecor1:
	case CellDecor2:
	case CellDecor3:
		player->speak(getid<celli>(p->type), getid<residi>(loc->type));
		break;
	case CellSecrectButton:
		if(change_overlay(party, party.d)) {
			party_addexp(400);
			player->speak("CellSecrectButton", "Accept");
			loc->state.secrets_found++;
		}
		break;
	case CellMessage:
		read_wall_messages(player, p);
		break;
	case CellCellar:
		if(*pi) {
			// Put item to cellar
			if(!pi->geti().is(Small))
				player->speak(getid<celli>(p->type), "NotFit");
			else
				loc->add(p, *pi);
		} else {
			// Get item from cellar
			item* items[1];
			if(loc->getitems(items, lenghtof(items), p)) {
				*pi = *items[0];
				items[0]->clear();
			} else
				player->speak(getid<celli>(p->type), "Empthy");
		}
		break;
	default:
		player->speak(getid<celli>(p->type), "About");
		break;
	}
	make_action();
}

static int get_side(const creaturei* p) {
	for(auto i = 0; i < 6; i++) {
		if(characters[i] == p) {
			if(i == 4)
				return 2;
			else if(i == 5)
				return 3;
			return i;
		}
	}
	return -1;
}

static directions get_drop(directions d) {
	static directions result[] = {Center, Right, Up, Left, Down};
	return result[d];
}

static void drop_dungeon_item() {
	auto pi = (item*)current_focus;
	auto pn = item_owner(pi);
	if(!pn)
		return;
	if(!can_remove(pi))
		return;
	loc->drop(party, *pi, get_side(get_side(pn), get_drop(party.d)));
}

static item* find_item_to_get(pointc v, directions d, int side) {
	dungeoni::ground* items[64];
	auto count = loc->getitems(items, lenghtof(items), v);
	if(!count)
		return 0;
	int sides[5];
	sides[0] = side;
	sides[1] = get_side(side, Left);
	sides[2] = get_side(side, Up);
	sides[3] = get_side(side, Down);
	sides[4] = get_side(side, Right);
	for(size_t r = 0; r < lenghtof(sides); r++) {
		auto s = sides[r];
		for(size_t i = 0; i < count; i++) {
			if(items[i]->side == s)
				return items[i];
		}
	}
	return 0;
}

void pick_up_dungeon_item() {
	auto pi = (item*)current_focus;
	auto pn = item_owner(pi);
	if(!pn || *pi)
		return;
	auto gpi = find_item_to_get(party, party.d, get_side(pn));
	if(!gpi)
		return;
	auto slot = item_wear(pi);
	if(!gpi->isallow(slot))
		return;
	if(slot >= Head && slot <= LastBelt) {
		if(!pn->isallow(*gpi))
			return;
	}
	*pi = *gpi;
	gpi->clear();
	consolen("%1 picked up", pi->getname());
}

static void play_dungeon_input() {
	static hotkeyi source[] = {
		{KeyLeft, move_party_left},
		{KeyRight, move_party_right},
		{KeyUp, move_party_up},
		{KeyDown, move_party_down},
		{KeyHome, turn_left},
		{KeyPageUp, turn_right},
		{'V', show_dungeon_automap},
		{'M', manipulate},
		{'D', drop_dungeon_item},
		{'T', test_dungeon},
		{KeyEscape, choose_dungeon_menu},
		{}};
	adventure_input(source);
	set_player_by_focus();
}

static void play_dungeon() {
	current_focus = save_focus;
	set_player_by_focus();
	show_scene(paint_adventure, play_dungeon_input, save_focus);
}

static void enter_active_dungeon() {
	set_dungeon_tiles(loc->type);
	make_action();
	save_focus = current_focus;
	set_next_scene(play_dungeon);
}

static void enter_dungeon(int bonus) {
	loc = find_dungeon(bonus);
	if(!loc && bonus == 0)
		bonus = 1;
	loc = find_dungeon(bonus);
	if(!loc)
		return;
	set_party_position(to(loc->state.up, loc->state.up.d), loc->state.up.d);
	enter_active_dungeon();
}

static bool party_move_interact(pointc v) {
	switch(loc->get(v)) {
	case CellStairsUp:
		if(find_dungeon(loc->level - 1))
			enter_dungeon(loc->level - 1);
		else if(confirm(getnm("ReturnToTownConfirm")))
			enter_location(0);
		break;
	case CellStairsDown:
		if(find_dungeon(loc->level + 1))
			enter_dungeon(loc->level + 1);
		else if(confirm(getnm("ReturnToTownConfirm")))
			enter_location(0);
		break;
	default:
		return false;
	}
	return true;
}

void move_party(pointc v) {
	if(!is_passable(v))
		return;
	if(loc->ismonster(v)) {
		turnto(v, to(party.d, Down));
		// TODO: interact with monsters
		pass_round();
		return;
	}
	if(party_move_interact(v))
		return;
	set_party_position(v);
	pass_round();
	explore_area();
}

static void party_adventure(int bonus) {
	choose_adventure();
	if(!last_quest)
		return;
	auto push_picture = picture;
	auto pn = getnme(ids(last_quest->id, "Summary"));
	if(pn) {
		if(!choose_dialog(pn, 0, getnm("Accept"), getnm("Decline"))) {
			picture = push_picture;
			return;
		}
	}
	pn = getnme(ids(last_quest->id, "Agree"));
	if(pn)
		message(pn);
	pn = getnme(ids(last_quest->id, "Entering"));
	if(pn)
		message(pn);
	picture = push_picture;
	enter_dungeon(0);
}

void continue_game() {
	last_location = bsdata<locationi>::elements + party.location;
	current_focus = 0;
	if(loc)
		enter_active_dungeon();
	else
		enter_location(0);
}

static void choose_spells(int bonus) {
	choose_spells("Spells available:", "Cancel", bonus);
}

static void identify_item(int bonus) {
	last_item->identify(bonus);
}

static void curse_item(int bonus) {
	last_item->curse(bonus);
}

static void learn_cleric_spells(int bonus) {
	pushanswer push;
	add_spells(0, 1, 0);
	for(auto& e : an.elements)
		player->knownspells.set(getbsi((spelli*)e.value));
}

static void pay_gold(int bonus) {
	bonus = get_bonus(bonus);
	if(getparty(GoldPiece) < bonus) {
		dialog(0, speech_get(get_action(), "NotEnoughtGold"), bonus);
		script_stop();
	} else
		add_party(GoldPiece, -bonus);
}

static void apply_racial_enemy(int bonus) {
	if(!last_race)
		return;
	if(bonus >= 0)
		player->hate.set(getbsi(last_race));
	else
		player->hate.remove(getbsi(last_race));
}

static void run_script(const char* id, const char* action) {
	auto p = bsdata<listi>::find(ids(id, action));
	if(p)
		ftscript<listi>(p - bsdata<listi>::elements, 0);
}

static void make_roll(int bonus) {
	if(!player->roll(last_ability, bonus * 5)) {
		script_stop();
		run_script(get_action(), "FailedRoll");
	}
}

static void dialog_message(const char* action) {
	dialog(0, speech_get(get_action(), action));
}

static void dialog_message(int bonus) {
	switch(bonus) {
	case 1: dialog_message("Success"); break;
	case -1: dialog_message("Fail"); break;
	}
}

static void player_name(stringbuilder& sb) {
	sb.add(player->getname());
}

static void effect_number(stringbuilder& sb) {
	sb.add("%1i", last_number);
}

static void dungeon_habbitant1(stringbuilder& sb) {
	sb.addv(getnm(getid<monsteri>(loc->habbits[0])), 0);
}

static void dungeon_habbitant2(stringbuilder& sb) {
	sb.addv(getnm(getid<monsteri>(loc->habbits[1])), 0);
}

static void stairs_down_side(stringbuilder& sb) {
	sb.addv(getnm(get_part_placement(loc->state.down)), 0);
}

static void stairs_up_side(stringbuilder& sb) {
	sb.addv(getnm(get_part_placement(loc->state.up)), 0);
}

bool parse_wall_messages(stringbuilder& sb, const char* id) {
	if(!loc)
		return false;
	auto pn = bsdata<wallmessagei>::find(id);
	if(!pn)
		return false;
	auto index = pn - bsdata<wallmessagei>::elements;
	sb.add("%1i", loc->state.wallmessages[index]);
	return true;
}

BSDATA(formulai) = {
	{"Add", add_formula},
	{"Mul", mul_formula},
	{"Set", set_formula},
	{"Sub", sub_formula},
};
BSDATAF(formulai)
BSDATA(textscript) = {
	{"Habbitant1", dungeon_habbitant1},
	{"Habbitant2", dungeon_habbitant2},
	{"Name", player_name},
	{"Number", effect_number},
	{"StairsDownSide", stairs_down_side},
	{"StairsUpSide", stairs_up_side},
};
BSDATAF(textscript)
BSDATA(script) = {
	{"Attack", attack_modify},
	{"ActivateQuest", activate_quest},
	{"ApplyAction", apply_action},
	{"ApplyRacialEnemy", apply_racial_enemy},
	{"ConfirmAction", confirm_action},
	{"ChooseSpells", choose_spells},
	{"ChooseMenu", choose_menu},
	{"CreateCharacter", create_character},
	{"CurseItem", curse_item},
	{"Damage", damage_modify},
	{"DoneQuest", done_quest},
	{"ExitGame", exit_game},
	{"EatAndDrink", eat_and_drink},
	{"EnterDungeon", enter_dungeon},
	{"EnterLocation", enter_location},
	{"IdentifyItem", identify_item},
	{"LearnClericSpells", learn_cleric_spells},
	{"LoadGame", load_game},
	{"Message", dialog_message},
	{"JoinParty", join_party},
	{"PartyAdventure", party_adventure},
	{"PayGold", pay_gold},
	{"ReturnToStreet", return_to_street},
	{"Roll", make_roll},
	{"SaveGame", save_game},
	{"Saves", saves_modify},
};
BSDATAF(script)
