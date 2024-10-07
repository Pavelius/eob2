#include "answers.h"
#include "class.h"
#include "collection.h"
#include "creature.h"
#include "draw.h"
#include "gender.h"
#include "hotkey.h"
#include "location.h"
#include "modifier.h"
#include "party.h"
#include "race.h"
#include "script.h"
#include "spell.h"
#include "textscript.h"
#include "view.h"
#include "view_focus.h"

extern "C" void exit(int code);

racei* last_race;
classi* last_class;
static void* last_result;

template<> void ftscript<racei>(int value, int bonus) {
	last_race = bsdata<racei>::elements + value;
}

template<> void ftscript<classi>(int value, int bonus) {
	last_class = bsdata<classi>::elements + value;
}

template<> void ftscript<genderi>(int value, int bonus) {
	last_gender = (gendern)value;
}

template<> void ftscript<locationi>(int value, int bonus) {
	last_location = bsdata<locationi>::elements + value;
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
	switch(modifier) {
	case Permanent:
		add_value(player->basic.abilities[value], bonus);
		break;
	default:
		add_value(player->abilities[value], bonus);
		break;
	}
}

template<> void ftscript<partystati>(int value, int bonus) {
	add_party((partystatn)value, bonus);
}

template<> void ftscript<itemi>(int value, int bonus) {
	item v; v.create(value);
	player->additem(v);
}

static void attack_modify(int bonus) {
	ftscript<abilityi>(AttackMelee, bonus);
	ftscript<abilityi>(AttackRange, bonus);
}

static void every_turn_effect() {
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

static void exit_game() {
	exit(0);
}

static void start_game() {
	exit(0);
}

static const char* get_header(const char* id, const char* group, const char* action) {
	auto pn = getnme(ids(id, action));
	if(!pn && group)
		pn = getnme(ids(group, action));
	if(!pn)
		pn = getnm(ids("Global", action));
	return pn;
}

static void add_menu(variant v, const char* action_id) {
	auto format = get_header(v.getid(), 0, action_id);
	an.add(v.getpointer(), format, v.getname(), 0, getnm(action_id));
}

static void enter_location(int bonus);

static void apply_result() {
	if(bsdata<locationi>::have(last_result)) {
		last_location = (locationi*)last_result;
		enter_location(0);
	} else if(bsdata<script>::have(last_result))
		((script*)last_result)->proc(0);
}

static void choose_options(const char* action, const char* id, const char* group, const variants& options) {
	// Function use huge amount of memory for existing copy of answers and return result to upper level.
	// So this memory used only for selection, not for each level of hierarhi.
	pushanswer push;
	char header[64]; stringbuilder sb(header);
	set_player_by_focus();
	sb.add(get_header(id, group, "Options"), getnm(id), getnm(group), getnm("Options"));
	for(auto v : options)
		add_menu(v, action);
	last_result = choose_answer(header, getnm("Cancel"), paint_city_menu, button_label, 1);
}

static void choose_city_menu() {
	auto& e = bsdata<locationi>::elements[party.location];
	choose_options("Visit", e.id, e.group, e.options);
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

static void play_location() {
	show_scene(paint_city, city_adventure_input);
}

static void enter_location(int bonus) {
	party.location = getbsi(last_location);
	picture = last_location->avatar;
	set_next_scene(play_location);
}

static void return_back(int bonus) {
	last_location = bsdata<locationi>::elements + party.location;
	if(last_location->parent) {
		last_location = last_location->parent;
		enter_location(0);
	}
}

static void debug_test(int bonus) {
	if(confirm("Really want quit game?"))
		exit(0);
}

static void gamble_visitors(int bonus) {
}

static void pick_pockets(int bonus) {
}

static void eat_and_drink(int bonus) {
}

static void memorize_spells(int bonus) {
	choose_spells("Spells available:", "Cancel", 0);
}

static void rest_party(int bonus) {
	if(!confirm(getnm("RestPartyConfirm")))
		return;
	return_back(0);
}

static void identify_item(int bonus) {
	last_item->identify(bonus);
}

static void curse_item(int bonus) {
	last_item->curse(bonus);
}

static bool cleric_spell(const void* object, int index) {
	return ((spelli*)object)->levels[0] == index;
}

static void learn_cleric_spells(int bonus) {
	collection<spelli> spells;
	spells.select();
	spells.match(cleric_spell, bonus, true);
	for(auto p : spells)
		player->spells.set(getbsi(p));
}

static void player_name(stringbuilder& sb) {
	sb.add(player->getname());
}

BSDATA(textscript) = {
	{"name", player_name},
};
BSDATAF(textscript)
BSDATA(script) = {
	{"Attack", attack_modify},
	{"CreateCharacter", create_character},
	{"CurseItem", curse_item},
	{"DebugTest", debug_test},
	{"EatAndDrink", eat_and_drink},
	{"EnterLocation", enter_location},
	{"GambleVisitors", gamble_visitors},
	{"IdentifyItem", identify_item},
	{"LearnClericSpells", learn_cleric_spells},
	{"MemorizeSpells", memorize_spells},
	{"JoinParty", join_party},
	{"PickPockets", pick_pockets},
	{"RestParty", rest_party},
	{"ReturnBack", return_back},
};
BSDATAF(script)