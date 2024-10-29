#include "action.h"
#include "alignment.h"
#include "answers.h"
#include "boost.h"
#include "cell.h"
#include "class.h"
#include "condition.h"
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
#include "math.h"
#include "modifier.h"
#include "monster.h"
#include "party.h"
#include "quest.h"
#include "race.h"
#include "rand.h"
#include "randomizer.h"
#include "resid.h"
#include "script.h"
#include "speech.h"
#include "spell.h"
#include "textscript.h"
#include "view.h"
#include "view_focus.h"
#include "wallmessage.h"

extern "C" void exit(int code);

static void* last_result;

static int get_bonus(int v) {
	switch(v) {
	case 101: return last_number;
	case -101: return -last_number;
	default: return v;
	}
}

template<> void ftscript<racei>(int value, int bonus) {
	last_race = (racen)value;
}

template<> void ftscript<classi>(int value, int bonus) {
	last_class = (classn)value;
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

template<> void ftscript<alignmenti>(int value, int bonus) {
	last_alignment = (alignmentn)value;
}

template<> void ftscript<actioni>(int value, int bonus) {
	last_action = bsdata<actioni>::elements + value;
}

template<> void ftscript<quest>(int value, int bonus) {
	last_quest = bsdata<quest>::elements + value;
}

template<> void ftscript<damagei>(int value, int bonus) {
	bonus = get_bonus(bonus);
	player->damage((damagen)value, bonus, 0);
}

template<> void ftscript<feati>(int value, int bonus) {
	switch(modifier) {
	case Permanent:
		player->basic.set((featn)value);
		break;
	default:
		player->set((featn)value);
		break;
	}
}

template<> void ftscript<abilityi>(int value, int bonus) {
	last_ability = (abilityn)value;
	if(!bonus)
		return;
	switch(modifier) {
	case Permanent: player->basic.add((abilityn)value, get_bonus(bonus)); break;
	default: player->add((abilityn)value, get_bonus(bonus)); break;
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
	auto id = party.quest_id;
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

static void damage_item(int bonus) {
	last_item->damage(bonus);
}

static void saves_modify(int bonus) {
	bonus *= 5;
	ftscript<abilityi>(SaveVsMagic, bonus);
	ftscript<abilityi>(SaveVsParalization, bonus);
	ftscript<abilityi>(SaveVsPoison, bonus);
	ftscript<abilityi>(SaveVsTraps, bonus);
}

static void create_new_game(int bonus) {
}

static void create_character(int bonus) {
	create_player();
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
	last_result = choose_large_menu(header, getnm("Cancel"));
}

static void choose_city_menu() {
	auto p = party.getlocation();
	choose_options("Visit", p->options);
	apply_result();
}

static void choose_dungeon_menu() {
	auto pi = bsdata<listi>::find("CampMenu");
	if(!pi)
		return;
	choose_options(pi->id, pi->elements);
	apply_result();
}

static void main_menu(const char* id) {
	pushanswer push;
	auto pi = bsdata<listi>::find(id);
	if(!pi)
		return;
	for(auto& v : pi->elements)
		add_menu(v);
	last_result = choose_main_menu();
	apply_result();
}

void main_menu() {
	main_menu("MainMenu");
	if(!is_next_scene())
		set_next_scene(main_menu);
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

static void player_heal(int bonus) {
	bonus = get_bonus(bonus);
	if(bonus <= 0)
		return;
	player->hp += bonus;
	if(player->hp > player->hpm)
		player->hp = player->hpm;
}

static void natural_heal(int bonus) {
	if(bonus >= 0)
		player_heal(xrand(1, 3) + bonus);
	else {
		if(d100() < 70)
			player->add(PoisonLevel, -bonus);
		else
			player->add(DiseaseLevel, -bonus);
		consolen(getnm("FeelWorse"));
	}
}

void restore_spells(int bonus) {
	auto spell_book = get_spells_prepared(player);
	if(spell_book)
		memcpy(player->spells, spell_book, sizeof(player->spells));
}

static void rest_character(int bonus) {
	natural_heal(bonus);
	restore_spells(0);
}

static void rest_party(int bonus) {
	auto push_player = player;
	for(auto p : characters) {
		if(!p)
			continue;
		player = p;
		rest_character(bonus);
	}
	player = push_player;
}

static dungeoni::overlayi* get_overlay() {
	if(!loc)
		return 0;
	return loc->get(party, party.d);
}

static void use_item() {
	last_item = (item*)current_focus;
	auto pn = item_owner(last_item);
	if(!pn)
		return;
	if(!pn->isactable())
		return;
	auto w = item_wear(last_item);
	auto& ei = last_item->geti();
	dungeoni::overlayi* po;
	switch(ei.wear) {
	case LeftHand:
	case RightHand:
		if(w != LeftHand && w != RightHand)
			pn->speak("MustBeUseInHand", 0);
		else if(last_item->isweapon()) {
			make_attacks();
			pass_round();
		}
		break;
	case Body: case Neck: case Elbow: case Legs: case Head:
	case LeftRing: case RightRing:
		pn->speak("MustBeWearing", 0);
		break;
	case Quiver:
		pn->speak("MustBeQuiver", 0);
		break;
	case Edible:
		if(last_item->isdamaged()) {
			player->speak("MakeCamp", "RottenFood");
			return;
		}
		if(confirm(getnm("MakeCampConfirm"))) {
			if(last_item->iscursed())
				rest_party(-2);
			else
				rest_party(last_item->geti().damage.roll());
			last_item->setcount(0);
		}
		break;
	case Readable:
		if(!pn->canread())
			pn->speak("CantRead", 0);
		else {
		}
		break;
	case Usable:
		if(!pn->isallow(*last_item)) {
			pn->speak("CantUse", 0);
			return;
		}
		script_run(last_item->geti().use);
		break;
	case Key:
		po = get_overlay();
		if(!po || po->type != CellKeyHole) {
			pn->speak("Key", "NoTargets");
			return;
		}
		if(!last_item->is(loc->getkey())) {
			pn->speak("Key", "WrongKey");
			return;
		}
		if(po->link) {
			consolen(getnm("DoorOpened"));
			loc->set(po->link, CellActive);
		}
		last_item->clear();
		break;
	default:
		break;
	}
}

static creaturei* choose_character(bool you) {
	pushanswer push_answer;
	for(auto p : characters) {
		if(!p || p->isdisabled())
			continue;
		if(p == player && !you)
			continue;
		an.add(p, p->getname());
	}
	return (creaturei*)choose_small_menu(getnm("WhatCharacter"), "Cancel");
}

static spelli* choose_spell(int level, int type) {
	pushanswer push_answer;
	for(auto& e : bsdata<spelli>()) {
		if(e.levels[type] != level)
			continue;
		an.add(&e, e.getname());
	}
	return (spelli*)choose_small_menu(getnm("WhatSpell"), "Cancel");
}

static void test_dungeon() {
	pointc v = party;
	for(int i = 0; i < 3; i++)
		v = to(v, party.d);
	thrown_item(v, Down, 6, get_party_index(player) % 2, 4);
}

static void city_adventure_input() {
	static hotkeyi keys[] = {
		{KeyEscape, choose_city_menu},
		{'E', cast_spell},
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

static bool mission_equipment(const char* id) {
	auto pi = bsdata<listi>::find(ids(id, "MissionEquipment"));
	if(!pi)
		return false;
	for(auto v : pi->elements) {
		if(v.iskind<itemi>()) {
			ftscript<itemi>(v.value, v.counter);
			last_item->tool(1);
			last_item->identify(1);
			last_item->curse(-1);
		}
	}
	return true;
}

static void craft_mission_equipment() {
	mission_equipment("Common");
	mission_equipment(player->getclassmain().id);
	mission_equipment(player->getrace().id);
}

static void clear_mission_equipment() {
	for(auto& e : player->wears) {
		if(e.istool())
			e.clear();
	}
}

static void enter_location(int bonus) {
	if(loc) {
		all_party(clear_mission_equipment, false);
	}
	loc = 0;
	last_quest = 0;
	party.quest_id = 0xFFFF;
	party.location_id = getbsi(last_location);
	picture = last_location->avatar;
	save_focus = current_focus;
	set_next_scene(play_location);
}

static void return_to_street(int bonus) {
	last_location = party.getlocation();
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

static void for_each_party(int bonus, const variants& commands, const slice<creaturei*>& characters) {
	auto push_player = player;
	for(auto p : characters) {
		if(!p)
			continue;
		switch(bonus) {
		case 0:
			if(p->isdisabled())
				continue;
			break;
		case -1:
			if(p->isdead())
				continue;
			break;
		default:
			break;
		}
		player = p;
		script_run(commands);
	}
	player = push_player;
}

static void for_each_party(int bonus) {
	variants commands; commands.set(script_begin, script_end - script_begin);
	for_each_party(bonus, commands, characters);
	script_stop();
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
	last_quest = (quest*)choose_large_menu(getnm("PartysAdventureAsk"), getnm("Cancel"));
	an = push_answers;
}

static bool choose_dialog(const char* format, const char* format_param, const char* yes, const char* no) {
	pushanswer push;
	an.add((void*)1, yes);
	return (bool)dialogv(no, format, format_param);
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
	if(!player->isunderstand(loc->language)) {
		player->speak("CellMessage", "Unrecognized");
		return;
	}
	auto p1 = getid<wallmessagei>(p->subtype);
	if(p->subtype < MessageHabbits) {
		if(loc->state.wallmessages[p->subtype] > 0)
			player->speak(p1, "Present");
		else
			player->speak(p1, "Miss");
	} else
		player->speak(p1, "Read");
}

static bool active_overlay(dungeoni::overlayi* p, bool test_linked) {
	if(p->is(CellActive) || (test_linked && p->link && loc->is(p->link, CellActive))) {
		player->speak(bsdata<celli>::elements[p->type].id, "Active");
		return true;
	}
	return false;
}

static bool use_tool_item(abilityn skill) {
	if(player->roll(skill))
		return true;
	consolen(getnm(ids(bsdata<abilityi>::elements[skill].id, "Fail")));
	auto chance = 60;
	if(d100() < chance) {
		auto tool_id = last_item->geti().id;
		last_item->damage(1);
		if(!(*last_item))
			consolen(getnm("ToolBroken"), getnm(tool_id));
	}
	return false;
}

static void use_tool_success(celln n) {
	player->addexp(100);
	consolen(getnm(ids(bsdata<celli>::elements[n].id, "Disable")));
}

static void use_theif_tools(int bonus) {
	auto p = loc->get(party, party.d);
	if(p) {
		switch(p->type) {
		case CellKeyHole:
			if(active_overlay(p, true))
				return;
			if(use_tool_item(OpenLocks)) {
				if(p->link) {
					use_tool_success(p->type);
					loc->set(p->link, CellActive);
				}
			}
			pass_round();
			return;
		case CellTrapLauncher:
			if(active_overlay(p, false))
				return;
			if(use_tool_item(RemoveTraps)) {
				use_tool_success(p->type);
				p->set(CellActive);
			}
			pass_round();
			return;
		}
	}
	switch(loc->get(to(party, party.d))) {
	case CellPit:
		if(use_tool_item(RemoveTraps)) {
			loc->set(to(party, party.d), CellPassable);
			use_tool_success(CellPit);
		}
		pass_round();
		return;
	}
	player->speak("TheifTool", "NoTargets");
}

static void manipulate() {
	auto v = to(party, party.d);
	auto player = item_owner(current_focus);
	if(!player)
		return;
	if(!player->isactable())
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
	if(!loc)
		return 0;
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
		{'U', use_item},
		{'E', cast_spell},
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

void monster_interaction() {
	make_melee_attacks();
}

void move_party(pointc v) {
	if(!is_passable(v))
		return;
	if(loc->ismonster(v)) {
		turnto(v, to(party.d, Down), true, -party_median(characters, Sneaky));
		monster_interaction();
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
	party.quest_id = getbsi(last_quest);
	all_party(craft_mission_equipment, true);
	enter_dungeon(0);
}

void continue_game() {
	last_quest = party.getquest();
	last_location = party.getlocation();
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
	auto ps = get_spells_known(player);
	if(!ps)
		return;
	pushanswer push;
	add_spells(0, 1, 0);
	for(auto& e : an.elements)
		ps->set(getbsi((spelli*)e.value));
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
		player->hate.set(last_race);
	else
		player->hate.remove(last_race);
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

static void get_last_random_effect(int bonus) {
	last_number = last_random_effect();
}

static void set_variable(int bonus) {
	party.abilities[last_variable] = get_bonus(bonus);
}

static void add_variable(int bonus) {
	party.abilities[last_variable] += get_bonus(bonus);
}

static void all_languages(int bonus) {
	if(bonus >= 0)
		player->languages = 0xFFFFFFFF;
	else
		player->languages = 0;
}

static void save_negate(int bonus) {
	if(player->roll(SaveVsMagic, bonus * 5)) {
		last_number = 0;
		script_stop();
	}
}

static void save_half(int bonus) {
	if(player->roll(SaveVsMagic, bonus * 5))
		last_number = last_number / 2;
}

static void set_character(int bonus) {
	player = characters[bonus];
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

static void dungeon_origin(stringbuilder& sb) {
	sb.addv(getnm(ids(getid<racei>(loc->language), "Of")), 0);
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

static bool if_alive() {
	return !player->isdead();
}

static bool if_wounded() {
	return player->hp < player->hpm;
}

static bool if_item_damaged() {
	return last_item->isdamaged();
}

static bool if_item_edible() {
	return last_item->is(Disease);
}

BSDATA(formulai) = {
	{"Add", add_formula},
	{"Mul", mul_formula},
	{"Set", set_formula},
	{"Sub", sub_formula},
};
BSDATAF(formulai)
BSDATA(textscript) = {
	{"DungeonOrigin", dungeon_origin},
	{"Habbitant1", dungeon_habbitant1},
	{"Habbitant2", dungeon_habbitant2},
	{"Name", player_name},
	{"Number", effect_number},
	{"StairsDownSide", stairs_down_side},
	{"StairsUpSide", stairs_up_side},
};
BSDATAF(textscript)
BSDATA(conditioni) = {
	{"IfAlive", if_alive},
	{"IfItemDamaged", if_item_damaged},
	{"IfItemEdible", if_item_edible},
	{"IfWounded", if_wounded},
};
BSDATAF(conditioni)
BSDATA(script) = {
	{"AllLanguages", all_languages},
	{"Attack", attack_modify},
	{"ActivateQuest", activate_quest},
	{"AddVariable", add_variable},
	{"ApplyAction", apply_action},
	{"ApplyRacialEnemy", apply_racial_enemy},
	{"ConfirmAction", confirm_action},
	{"Character", set_character},
	{"ChooseSpells", choose_spells},
	{"ChooseMenu", choose_menu},
	{"CreateCharacter", create_character},
	{"CreateNewGame", create_new_game},
	{"CurseItem", curse_item},
	{"Damage", damage_modify},
	{"DamageItem", damage_item},
	{"DoneQuest", done_quest},
	{"ExitGame", exit_game},
	{"EatAndDrink", eat_and_drink},
	{"EnterDungeon", enter_dungeon},
	{"EnterLocation", enter_location},
	{"ForEachParty", for_each_party},
	{"Heal", player_heal},
	{"IdentifyItem", identify_item},
	{"LearnClericSpells", learn_cleric_spells},
	{"LoadGame", load_game},
	{"Message", dialog_message},
	{"NaturalHeal", natural_heal},
	{"JoinParty", join_party},
	{"PartyAdventure", party_adventure},
	{"PayGold", pay_gold},
	{"PassHours", pass_hours},
	{"RandomEffect", get_last_random_effect},
	{"RestoreSpells", restore_spells},
	{"ReturnToStreet", return_to_street},
	{"Roll", make_roll},
	{"SaveGame", save_game},
	{"Saves", saves_modify},
	{"SaveHalf", save_half},
	{"SaveNegate", save_negate},
	{"SetVariable", set_variable},
	{"UseTheifTool", use_theif_tools},
};
BSDATAF(script)
