#include "action.h"
#include "answers.h"
#include "assign.h"
#include "class.h"
#include "creature.h"
#include "draw.h"
#include "dungeon.h"
#include "formula.h"
#include "gender.h"
#include "hotkey.h"
#include "list.h"
#include "location.h"
#include "modifier.h"
#include "party.h"
#include "quest.h"
#include "race.h"
#include "script.h"
#include "speech.h"
#include "spell.h"
#include "textscript.h"
#include "view.h"
#include "view_focus.h"

extern "C" void exit(int code);

racei* last_race;
classi* last_class;
static void* last_result;

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

static const char* get_action() {
	if(last_action)
		return last_action->id;
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

void skip_hours(int value) {
	add_party(Minutes, 60 * value);
}

static void join_party(int bonus) {
	for(auto& e : party.units) {
		if(e)
			continue;
		e = player;
		break;
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

static void add_menu(variant& v, const char* action_id) {
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
	} else {
		auto format = get_header(v.getid(), action_id);
		an.add(v.getpointer(), format, v.getname(), getnm(action_id));
	}
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
	else if(bsdata<actioni>::have(last_result)) {
		last_action = (actioni*)last_result;
		apply_action(0);
	} else if(bsdata<variant>::have(last_result))
		script_run(*((variant*)last_result));
}

static void choose_options(const char* action, const char* id, const variants& options) {
	// Function use huge amount of memory for existing copy of answers and return result to upper level.
	// So this memory used only for selection, not for each level of hierarhi.
	pushanswer push;
	char header[64]; stringbuilder sb(header);
	set_player_by_focus();
	sb.add(get_header(id, "Options"), getnm(id), getnm("Options"));
	for(auto& v : options)
		add_menu(v, action);
	last_result = choose_answer(header, getnm("Cancel"), paint_city_menu, button_label, 1);
}

static void choose_city_menu() {
	auto& e = bsdata<locationi>::elements[party.location];
	choose_options("Visit", e.id, e.options);
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

static void city_adventure_input() {
	static hotkeyi keys[] = {
		{KeyEscape, choose_city_menu},
		{'D', drop_city_item},
		{'U', use_item},
		{}};
	city_input(keys);
}

static void* save_focus;

static void play_location() {
	show_scene(paint_city, city_adventure_input, save_focus);
}

static void enter_location(int bonus) {
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
	an.clear();
	auto id = get_action();
	for(auto& v : commands)
		add_menu(v, id);
	last_result = choose_answer(getnm(id), getnm("Cancel"), paint_city_menu, button_label, 1);
	an.clear();
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

static void play_dungeon() {
	show_scene(paint_adventure, city_adventure_input, save_focus);
}

static dungeoni* find_dungeon(quest* pv) {
	auto id = getbsi(pv);
	if(id == 0xFFFF)
		return 0;
	for(auto& e : bsdata<dungeoni>()) {
		if(e.quest_id == id)
			return &e;
	}
	return 0;
}

static void enter_dungeon(int bonus) {
	loc = find_dungeon(last_quest);
	if(!loc)
		return;
	set_dungeon_tiles(loc->type);
	save_focus = current_focus;
	assign<pointc>(party, to(loc->state.up, loc->state.up.d));
	party.d = loc->state.up.d;
	animation_update();
	set_next_scene(play_dungeon);
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
	current_focus = 0;
	last_location = bsdata<locationi>::elements + party.location;
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

static void test_dungeon(int bonus) {
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

BSDATA(formulai) = {
	{"Add", add_formula},
	{"Mul", mul_formula},
	{"Set", set_formula},
	{"Sub", sub_formula},
};
BSDATAF(formulai)
BSDATA(textscript) = {
	{"Name", player_name},
	{"Number", effect_number},
};
BSDATAF(textscript)
BSDATA(script) = {
	{"Attack", attack_modify},
	{"ActivateQuest", activate_quest},
	{"ApplyAction", apply_action},
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
	{"TestDungeon", test_dungeon},
};
BSDATAF(script)
