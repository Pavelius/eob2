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
#include "keyvalue.h"
#include "list.h"
#include "location.h"
#include "math.h"
#include "modifier.h"
#include "monster.h"
#include "party.h"
#include "pointca.h"
#include "pushvalue.h"
#include "quest.h"
#include "race.h"
#include "rand.h"
#include "randomizer.h"
#include "resid.h"
#include "script.h"
#include "screenshoot.h"
#include "shop.h"
#include "speech.h"
#include "spell.h"
#include "talking.h"
#include "textscript.h"
#include "view.h"
#include "view_focus.h"
#include "wallmessage.h"

extern "C" void exit(int code);

static void* last_result;
static void* save_focus;

static int get_bonus(int v) {
	switch(v) {
	case 101: return last_number;
	case 102: return last_level;
	case -101: return -last_number;
	case -102: return -last_level;
	default: return v;
	}
}

template<> void ftscript<racei>(int value, int bonus) {
	last_race = (racen)value;
}

template<> void ftscript<classi>(int value, int bonus) {
	last_class = (classn)value;
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
	player->damage((damagen)value, get_bonus(bonus));
}

template<> void ftscript<reactioni>(int value, int bonus) {
	last_reaction = (reactions)value;
}

template<> bool fttest<feati>(int value, int bonus) {
	return player->is((featn)value) == (bonus >= 0);
}
template<> void ftscript<feati>(int value, int bonus) {
	switch(modifier) {
	case Permanent: player->basic.set((featn)value, bonus >= 0); break;
	default: player->set((featn)value, bonus >= 0); break;
	}
}

template<> bool fttest<shopi>(int value, int bonus) {
	return bsdata<shopi>::elements[value].getsize() != 0;
}
template<> void ftscript<shopi>(int value, int bonus) {
	last_shop = bsdata<shopi>::elements + value;
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
	v.createpower(bonus, bonus ? 100 : 0, 0);
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

static void destroy_item(int bonus) {
	last_item->clear();
}

static void saves_modify(int bonus) {
	bonus *= 5;
	ftscript<abilityi>(SaveVsMagic, bonus);
	ftscript<abilityi>(SaveVsParalization, bonus);
	ftscript<abilityi>(SaveVsPoison, bonus);
	ftscript<abilityi>(SaveVsTraps, bonus);
}

static void protection_modify(int bonus) {
	ftscript<abilityi>(AC, bonus);
	saves_modify(bonus);
}

static void wizardy_effect(int bonus) {
	switch(bonus) {
	case 1: ftscript<abilityi>(Spell1, player->basic.abilities[Spell1]); break;
	case 2: ftscript<abilityi>(Spell2, player->basic.abilities[Spell2]); break;
	case 3: ftscript<abilityi>(Spell3, player->basic.abilities[Spell3]); break;
	default: break;
	}
}

static void select_items(conditioni::fntest proc, bool keep) {
	auto push_item = last_item;
	for(auto p : characters) {
		if(!p)
			continue;
		for(auto& it : p->wears) {
			if(!it)
				continue;
			last_item = &it;
			if(proc() != keep)
				continue;
			an.add(&it, it.getname());
		}
	}
	last_item = push_item;
}

static void filter_items(conditioni::fntest proc, bool keep) {
	auto ps = an.begin();
	auto pe = an.end();
	auto push_item = last_item;
	for(auto pb = an.begin(); pb < pe; pb++) {
		last_item = (item*)pb->value;
		if(proc() != keep)
			continue;
		*ps++ = *pb;
	}
	an.elements.count = ps - an.begin();
	last_item = push_item;
}

static void create_character(int bonus) {
	create_player();
}

static const char* get_header(const char* id, const char* action) {
	const char* pn = 0;
	if(id)
		pn = getnme(ids(id, action));
	if(!pn && action)
		pn = getnm(ids("Global", action));
	return pn;
}

static const char* get_title(const char* id, const char* action) {
	static char temp[128];
	auto pn = get_header(id, action);
	if(!pn)
		return 0;
	stringbuilder sb(temp);
	sb.clear();
	sb.add(pn);
	return temp;
}

static bool ismatch(char* abilitites, int* resources) {
	for(auto i = 0; i <= Blessing; i++) {
		if(abilitites[i] && resources[i] < abilitites[i])
			return false;
	}
	return true;
}

bool ismatch(char* abilitites) {
	return ismatch(abilitites, party.abilities);
}

static void add_menu(variant& v, bool whole_party = false) {
	if(v.iskind<actioni>()) {
		auto p = bsdata<actioni>::elements + v.value;
		if(p->restrict_classes && party_have(p->restrict_classes))
			return;
		if(!ismatch(p->required))
			return;
		if(whole_party) {
			if(p->classes && !party_have(p->classes))
				return;
			if(!allow_item(p->filter_item))
				return;
		} else {
			if(p->races && !p->races.is(player->race))
				return;
			if(p->classes && !have_class(p->classes, player->character_class))
				return;
			if(p->alignment && !p->alignment.is(player->alignment))
				return;
			if(p->filter && !script_allow(p->filter))
				return;
			if(p->filter_item && !allow_item(player, p->filter_item))
				return;
		}
		an.add(v.getpointer(), v.getname());
	} else if(v.iskind<formulai>()) {
		auto p = bsdata<formulai>::elements + v.value;
		an.add(&v, getnm(ids(p->id, last_id)), p->proc(last_number, v.counter));
	} else if(v.iskind<script>()) {
		auto p = bsdata<script>::elements + v.value;
		an.add(p, getnm(p->id));
	} else if(v.iskind<locationi>()) {
		auto format = get_header(v.getid(), "Visit");
		if(!ismatch(bsdata<locationi>::elements[v.value].required))
			return;
		an.add(v.getpointer(), format, v.getname());
	} else
		an.add(v.getpointer(), v.getname());
}

static void enter_location(int bonus);

static void apply_action(int bonus) {
	if(last_action->avatar)
		picture = last_action->avatar;
	script_run(last_action->id, last_action->effect);
}

static void apply_result() {
	if(!last_result) {
		last_number = 0;
	} else if(bsdata<locationi>::have(last_result)) {
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
	fix_animate();
}

static void choose_options(const char* id, const variants& options) {
	// Function use huge amount of memory for existing copy of answers and return result to upper level.
	// So this memory used only for selection, not for each level of hierarhi.
	pushanswer push;
	char header[64]; stringbuilder sb(header);
	set_player_by_focus();
	sb.add(get_header(id, "Options"));
	for(auto& v : options)
		add_menu(v);
	last_result = choose_large_menu(header, getnm("Cancel"));
}

static void choose_item(const char* id, const variants& filter) {
	pushanswer push;
	char header[64]; stringbuilder sb(header);
	set_player_by_focus();
	sb.add(get_header(id, "Options"));
	for(auto& e : player->wears) {
		last_item = &e;
		if(!script_allow(filter))
			continue;
		an.add(&e, e.getname());
	}
	last_item = (item*)choose_large_menu(header, getnm("Cancel"));
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
	if(pi->is(QuestItem))
		return;
	char temp[128]; stringbuilder sb(temp);
	sb.add(getnm("SellConfirm"), pi->getname(), cost);
	if(!confirm(temp))
		return;
	pi->clear();
	add_party(GoldPiece, cost);
}

static void player_add_aid(int bonus) {
	player->hp_aid += get_bonus(bonus);
	if(player->hp_aid < 0)
		player->hp_aid = 0;
}

static void player_heal(int bonus) {
	player->heal(get_bonus(bonus));
}

static void player_heal_effect(int bonus) {
	static dice effect[] = {
		{1, 3}, // Small remedy
		{2, 4, 2}, // Potion of healing
		{3, 8, 3}, // Potion of extra-healing
	};
	player->heal(maptbl(effect, bonus).roll());
}

static void natural_heal(int bonus) {
	if(bonus >= 0)
		player_heal(xrand(1, 3) + bonus);
	else {
		if(d100() < 70)
			player->add(PoisonLevel, -bonus * 3);
		else
			player->add(DiseaseLevel, -bonus);
		consolen(getnm("FeelWorse"));
	}
}

static void add_type_spells(variant type) {
	for(auto& e : bsdata<keyvaluei>()) {
		if(e.key.value != type.value || e.key.counter > type.counter)
			continue;
		if(e.value.iskind<spelli>())
			player->spells[e.value.value] += e.value.counter;
	}
}

static void update_class_spells() {
	auto& ei = player->getclass();
	variant type = "Fighter";
	if(!type)
		return;
	for(auto i = 0; i < ei.count; i++) {
		type.value = ei.classes[i];
		type.counter = player->levels[i];
		add_type_spells(type);
	}
}

static void update_camp_spells() {
	for(auto& e : player->equipment()) {
		if(!e)
			continue;
		auto w = e.geti().wear;
		if(w == RightHand || w == LeftHand || w == Rod)
			continue;
		auto power = e.getpower();
		if(!power)
			continue;
		if(power.iskind<spelli>())
			player->spells[power.value] += power.counter;
	}
}

static void update_memorized_spells() {
	auto spell_book = get_spells_prepared(player);
	if(spell_book)
		memcpy(player->spells, spell_book, sizeof(player->spells));
	else
		memset(player->spells, 0, sizeof(player->spells));
}

void restore_spells(int bonus) {
	update_memorized_spells();
	update_camp_spells();
	update_class_spells();
}

static void satisfy(int bonus) {
	if(!bonus) {
		auto maximum = player->getfood();
		if(player->food < maximum)
			player->food = maximum;
	} else
		player->food += bonus * 10;
}

static void sleep_character(int bonus) {
	natural_heal(bonus);
	satisfy(0);
	restore_spells(0);
}

static void sleep_party(int bonus) {
	auto push_player = player;
	for(auto p : characters) {
		if(!p)
			continue;
		player = p;
		sleep_character(bonus);
	}
	player = push_player;
	pass_hours(8);
}

static dungeoni::overlayi* get_overlay() {
	if(!loc)
		return 0;
	return loc->get(party, party.d);
}

static void apply_script(const char* action, const char* id, int bonus) {
	pushvalue push(last_id);
	auto sid = ids(action, id);
	auto p1 = bsdata<script>::find(sid);
	if(p1) {
		last_id = p1->id;
		p1->proc(bonus);
		return;
	}
	auto p2 = bsdata<listi>::find(sid);
	if(p2) {
		last_id = p2->id;
		script_run(p2->elements);
		return;
	}
	auto p3 = bsdata<randomizeri>::find(sid);
	if(p3) {
		last_id = p3->id;
		auto v = single(p3->random());
		v.counter = bonus;
		script_run(v);
		return;
	}
}

static int get_permanent_raise(creaturei* player, abilityn a, int magical_bonus) {
	if(a >= Strenght && a <= Charisma)
		return 4;
	return 10;
}

static int get_ability_number(creaturei* player, abilityn a, int magical_bonus) {
	switch(a) {
	case Strenght: case Dexterity: case Constitution:
	case Intellegence: case Wisdow: case Charisma:
		if(magical_bonus < 0)
			return 3;
		return 17 + magical_bonus - player->basic.abilities[a];
	case AC: case AttackMelee: case AttackRange: case DamageMelee: case DamageRange:
	case Speed: case Backstab: case TurnUndeadBonus:
		return magical_bonus;
	case SaveVsMagic: case SaveVsParalization: case SaveVsTraps: case SaveVsPoison:
		return magical_bonus * 5;
	default:
		switch(magical_bonus) {
		case -1: return -20;
		case -2: return -40;
		case -3: return -60;
		case -4: return -80;
		case -5: return -100;
		case 0: return 40;
		default: return 100;
		}
	}
}

static void drink_effect(variant v, unsigned duration, int multiplier) {
	if(v.iskind<listi>()) {
		for(auto e : bsdata<listi>::elements[v.value].elements)
			drink_effect(e, duration, multiplier);
	} else if(v.iskind<script>()) {
		if(multiplier < 0)
			player->add(PoisonLevel, xrand(3, 6));
		else
			bsdata<script>::elements[v.value].proc(v.counter);
	} else if(v.iskind<abilityi>()) {
		auto permanent = get_permanent_raise(player, (abilityn)v.value, v.counter);
		if(v.counter >= permanent)
			player->basic.add((abilityn)v.value, (v.counter - permanent + 1) * multiplier);
		else {
			v.counter = get_ability_number(player, (abilityn)v.value, v.counter * multiplier);
			if(v.counter)
				add_boost(party.abilities[Minutes] + duration, player, v);
		}
	} else if(v.iskind<feati>()) {
		v.counter = multiplier;
		add_boost(party.abilities[Minutes] + duration, player, v);
	}
}

static bool read_effect(creaturei* pn, variant v, int experience, unsigned duration) {
	bool result = false;
	last_number = duration;
	auto push_player = player; player = pn;
	if(v.iskind<spelli>()) {
		result = cast_spell(bsdata<spelli>::elements + v.value, player->getlevel() + v.counter, experience, true, false, 0);
		if(result)
			pass_round();
	} else if(v.iskind<listi>() || v.iskind<randomizeri>() || v.iskind<script>()) {
		script_run(v);
		last_item->usecharge("ConsumeTome", 40, 5);
		pass_hours(1);
	}
	player = push_player;
	return result;
}

static bool use_rod(creaturei* pn, item* rod, variant v) {
	bool result = false;
	if(v.iskind<spelli>()) {
		auto push_player = player; player = pn;
		auto ps = bsdata<spelli>::elements + v.value;
		result = cast_spell(ps, 1 + v.counter * 2, 0, true, false, 0);
		if(result) {
			consolen("%Name cast %1", ps->getname());
			rod->identify(1);
		}
		player = push_player;
	}
	if(result)
		rod->usecharge("ConsumeRod");
	return result;
}

static void drink_effect(creaturei* pn, variant v, unsigned duration, int multiplier) {
	auto push_player = player; player = pn;
	drink_effect(v, duration, multiplier);
	update_player();
	player = push_player;
}

static bool allow_use(creaturei* player, item* p) {
	if(!player->isallow(*p)) {
		player->speak("CantUse", 0);
		return false;
	}
	return true;
}

static bool dungeon_use() {
	if(!loc) {
		player->speak("CantUse", "City");
		return false;
	}
	return true;
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
		if(!dungeon_use())
			break;
		if(w != LeftHand && w != RightHand)
			pn->speak("MustBeUseInHand", 0);
		else if(last_item->isweapon()) {
			make_attacks(false);
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
	case Drinkable:
		if(!allow_use(pn, last_item))
			break;
		drink_effect(pn, last_item->getpower(), xrand(5, 20) * 10, last_item->iscursed() ? -1 : 1);
		consolen(getnm("DrinkPotionAct"));
		last_item->clear();
		break;
	case Edible:
		if(!allow_use(pn, last_item))
			break;
		if(!dungeon_use())
			break;
		if(last_item->isdamaged()) {
			player->speak("MakeCamp", "RottenFood");
			break;
		}
		if(confirm(getnm("MakeCampConfirm"))) {
			if(last_item->iscursed())
				sleep_party(-2);
			else
				sleep_party(last_item->geti().damage.roll());
			last_item->clear();
		}
		break;
	case Readable:
		if(!allow_use(pn, last_item))
			break;
		if(!pn->canread())
			pn->speak("CantRead", 0);
		else {
			if(read_effect(pn, last_item->getpower(), 50, xrand(5, 20) * 10))
				last_item->clear();
		}
		break;
	case Rod:
		if(!allow_use(pn, last_item))
			break;
		if(w != RightHand) {
			pn->speak("MustBeWearing", "RightHand");
			break;
		}
		if(use_rod(pn, last_item, last_item->getpower()))
			pass_round();
		break;
	case Usable:
		if(!allow_use(pn, last_item))
			break;
		apply_script("Use", last_item->geti().id, last_item->iscursed() ? -2 : last_item->getpower().counter);
		break;
	case Key:
		if(!dungeon_use())
			break;
		po = get_overlay();
		if(!po || po->type != CellKeyHole) {
			pn->speak("Key", "NoTargets");
			break;
		}
		if(!last_item->is(loc->getkey())) {
			pn->speak("Key", "WrongKey");
			break;
		}
		if(po->link) {
			consolen(getnm("DoorOpened"));
			loc->set(po->link, CellActive);
			loc->state.locks_open++;
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
	an.sort();
	return (spelli*)choose_small_menu(getnm("WhatSpell"), "Cancel");
}

static void test_dungeon() {
	pointc v = party;
	for(int i = 0; i < 3; i++)
		v = to(v, party.d);
	thrown_item(v, Down, 6, get_party_index(player) % 2, 4);
}

static void change_quick_item() {
	auto pi = (item*)current_focus;
	auto pn = item_owner(pi);
	auto w = item_wear(pi);
	if(!pn || w != RightHand)
		return;
	if(!pn->wears[FirstBelt]) {
		pn->speak("NoQuickItem", 0);
		return;
	}
	if(!pn->wears[FirstBelt].isallow(w))
		return;
	if(!pn->isallow(pn->wears[FirstBelt]))
		return;
	if(!can_remove(pi, true))
		return;
	auto it = *pi;
	*pi = pn->wears[FirstBelt];
	memmove(pn->wears + FirstBelt, pn->wears + SecondBelt, sizeof(it) * 2);
	pn->wears[LastBelt].clear();
	for(auto i = FirstBelt; i <= LastBelt; i = (wearn)(i + 1)) {
		if(!pn->wears[i]) {
			pn->wears[i] = it;
			break;
		}
	}
}

static void city_adventure_input() {
	static hotkeyi keys[] = {
		{KeyEscape, choose_city_menu},
		{'E', cast_spell},
		{'D', drop_city_item},
		{'U', use_item},
		{'R', change_quick_item},
		{'T', test_dungeon},
		{}};
	city_input(keys);
}

static void message(const char* format, const char* format_param = 0) {
	pushanswer push;
	dialogv(0, format);
}

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
			last_item->set(ToolItem);
			last_item->identify(1);
			last_item->curse(-1);
		} else
			last_item->setpower(v);
	}
	return true;
}

static void craft_mission_equipment() {
	mission_equipment("Common");
	auto& ei = player->getclass();
	for(auto i = 0; i < ei.count; i++)
		mission_equipment(bsdata<classi>::elements[ei.classes[i]].id);
	mission_equipment(player->getrace().id);
}

static void clear_mission_equipment() {
	for(auto& e : player->wears) {
		if(e.is(ToolItem))
			e.clear();
	}
}

static void clear_edible() {
	for(auto& e : player->wears) {
		if(e.is(Edible))
			e.clear();
	}
}

static bool last_quest_complite() {
	if(!last_quest)
		return false;
	auto quest_id = getbsi(last_quest);
	for(auto i = (goaln)0; i <= KillAlmostAllMonsters; i = (goaln)(i + 1)) {
		if(!last_quest->goals[i])
			continue;
		auto value = party_goal(quest_id, i);
		if(value < last_quest->goals[i])
			return false;
	}
	return true;
}

static void check_quest_complited() {
	if(!last_quest_complite())
		return;
	auto push_quest = last_quest;
	auto pn = getnme(ids(last_quest->id, "Finish"));
	if(pn)
		message(pn);
	script_run(last_quest->reward);
	last_quest = push_quest;
	party.done.set(getbsi(last_quest));
}

static void loot_selling() {
	for(auto& e : player->backpack()) {
		if(!e || e.ismagical())
			continue;
		if(e.is(QuestItem))
			continue;
		auto& ei = e.geti();
		if(ei.wear == Key || ei.wear == Readable)
			continue;
		auto value = e.geti().cost; // Full price??
		last_number += value;
		party.abilities[GoldPiece] += value;
		e.clear();
	}
}

static void loot_identyfing() {
	for(auto& e : player->wears) {
		if(!e || e.isidentified())
			continue;
		e.identify(1);
		if(e.ismagical())
			last_number++;
	}
}

static void after_dungeon_action(const char* id, fnevent proc) {
	last_number = 0;
	all_party(proc, false);
	if(last_number)
		dialog(0, getnm(id));
}

static void copy_ability(int* dest, int* source) {
	for(auto i = 0; i <= Blessing; i++) {
		if(dest[i] < source[i])
			dest[i] = source[i];
	}
}

static void party_unlock() {
	// Unlock locations
	for(auto& e : bsdata<locationi>()) {
		if(!ismatch(e.required))
			continue;
		if(ismatch(e.required, party.unlock))
			continue;
		auto pn = getnme(ids(e.id, "Unlock"));
		if(!pn)
			continue;
		dialog(0, pn);
	}
	// Unlock actions
	for(auto& e : bsdata<actioni>()) {
		if(!ismatch(e.required))
			continue;
		if(ismatch(e.required, party.unlock))
			continue;
		auto pn = getnme(ids(e.id, "Unlock"));
		if(!pn)
			continue;
		dialog(0, pn);
	}
	// Copy unlock ability
	copy_ability(party.unlock, party.abilities);
}

static void enter_location(int bonus) {
	if(loc) {
		all_party(clear_mission_equipment, false);
		all_party(clear_edible, false);
		check_quest_complited();
		after_dungeon_action("LootIdentyfing", loot_identyfing);
		after_dungeon_action("LootSelling", loot_selling);
		pass_hours(xrand(2, 4));
	}
	party_unlock();
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
	char temp[260]; stringbuilder sb(temp);
	sb.add(getnme(ids(last_id, "Confirm")));
	if(!temp[0] || !confirm(temp))
		script_stop();
}

static void exit_game(int bonus) {
	if(confirm(getnme("ExitConfirm")))
		exit(0);
}

static void save_game(int bonus) {
	save_game("autosave");
}

static void load_game(int bonus) {
	read_game("autosave");
}

static void choose_menu(int bonus) {
	scriptbody commands;
	choose_options(last_id, commands);
	apply_result();
	script_stop();
}

static bool confirm_payment(const char* name, int gold_coins) {
	if(getparty(GoldPiece) < gold_coins) {
		dialog(0, speech_get(last_id, "NotEnoughtGold"), gold_coins);
		return false;
	} else {
		char temp[260]; stringbuilder sb(temp);
		sb.add(getnm(ids(last_id, "Confirm")), name, gold_coins);
		if(!confirm(temp))
			return false;
		add_party(GoldPiece, -gold_coins);
	}
	return true;
}

static void buy_menu(int bonus) {
	scriptbody commands;
	choose_options(last_id, commands);
	script_stop();
	if(!last_result)
		return;
	if(bsdata<itemi>::source.have(last_result)) {
		auto p = (itemi*)last_result;
		if(!confirm_payment(getnm(p->id), p->cost))
			return;
		item it; it.create(p);
		it.identify(1);
		player->wearable::additem(it);
	}
}

static void choose_items(int bonus) {
	scriptbody commands;
	choose_item(last_id, commands);
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

static void for_each_item(int bonus) {
	scriptbody commands;
	auto push_item = last_item;
	for(auto& e : an) {
		last_item = (item*)e.value;
		script_run(commands);
	}
	last_item = push_item;
	script_stop();
}

static void for_each_party(int bonus) {
	scriptbody commands;
	for_each_party(bonus, commands, characters);
	script_stop();
}

static void activate_linked_overlay(int bonus) {
	auto po = loc->get(*player, player->d);
	if(!po || !po->link)
		return;
	loc->set(po->link, CellActive);
}

static int compare_item_cost(const void* v1, const void* v2) {
	auto p1 = *((item**)v1);
	auto p2 = *((item**)v2);
	return p1->geti().cost - p2->geti().cost;
}

static void action_items(int bonus) {
	an.clear();
	if(!last_action || !last_action->filter_item)
		return;
	auto push = last_item;
	for(auto p : characters) {
		if(!p || !p->isready())
			continue;
		for(auto& e : p->wears) {
			if(!e)
				continue;
			last_item = &e;
			if(!script_allow(last_action->filter_item))
				continue;
			an.add(&e, e.getname());
		}
	}
	qsort(an.elements.data, an.elements.count, sizeof(an.elements.data[0]), compare_item_cost);
	if(bonus > 0 && an.elements.count > (size_t)bonus)
		an.elements.count = bonus;
	last_item = push;
}

static void action_player_items(int bonus) {
	an.clear();
	if(!last_action || !last_action->filter_item)
		return;
	auto push = last_item;
	for(auto& e : player->wears) {
		if(!e)
			continue;
		last_item = &e;
		if(!script_allow(last_action->filter_item))
			continue;
		an.add(&e, e.getname());
	}
	qsort(an.elements.data, an.elements.count, sizeof(an.elements.data[0]), compare_item_cost);
	if(bonus > 0 && an.elements.count > (size_t)bonus)
		an.elements.count = bonus;
	last_item = push;
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

static void choose_quest() {
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
	last_quest = (quest*)choose_large_menu(get_title("PartyAdventure", "Options"), getnm("Cancel"));
	an = push_answers;
}

static bool choose_dialog(const char* format, const char* format_param, const char* yes, const char* no) {
	pushanswer push;
	an.add((void*)1, yes);
	return (bool)dialogv(no, format, format_param);
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
	last_item->usecharge("ToolBroken");
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
					loc->state.locks_open++;
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
				loc->state.traps_disabled++;
			}
			pass_round();
			return;
		default:
			break;
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
	default:
		break;
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
	case CellSecretButton:
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
		{'R', change_quick_item},
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

static void enter_dungeon_from_up(int bonus) {
	loc = find_dungeon(bonus);
	if(!loc && bonus == 0)
		bonus = 1;
	loc = find_dungeon(bonus);
	if(!loc)
		return;
	set_party_position(to(loc->state.down, loc->state.down.d), loc->state.down.d);
	enter_active_dungeon();
}

static void pit_fall_down() {
	if(!player->roll(ClimbWalls))
		player->damage(Bludgeon, xrand(3, 18));
}

static bool party_move_interact(pointc v) {
	switch(loc->get(v)) {
	case CellStairsUp:
		if(find_dungeon(loc->level - 1)) {
			enter_dungeon_from_up(loc->level - 1);
			consolen(getnm("PartyGoingUp"));
		} else if(confirm(getnm("ReturnToTownConfirm")))
			enter_location(0);
		break;
	case CellStairsDown:
		if(find_dungeon(loc->level + 1)) {
			enter_dungeon(loc->level + 1);
			consolen(getnm("PartyGoingDown"));
		} else if(confirm(getnm("ReturnToTownConfirm")))
			enter_location(0);
		break;
	case CellPit:
		loc = find_dungeon(loc->level + 1);
		animation_update();
		all_party(pit_fall_down, true);
		consolen(getnm("PartyFallPit"));
		pass_round();
		break;
	default:
		return false;
	}
	return true;
}

static creaturei* get_leader(creaturei** creatures) {
	for(auto i = 0; i < 6; i++) {
		if(creatures[i])
			return creatures[i];
	}
	return 0;
}

static void talk_monsters(const char* format) {
	auto pm = opponent->getmonster();
	if(!pm)
		return;
	auto pq = party.getquest();
	if(!pq)
		return;
	auto rm = bsdata<reactioni>::elements[last_reaction].id;
	pushanswer push;
	auto pe = bsdata<listi>::find(ids(pq->id, rm));
	if(!pe && opponent->isanimal())
		pe = bsdata<listi>::find(ids("Animal", rm));
	if(!pe)
		pe = bsdata<listi>::find(ids("Negotiation", rm));
	if(!pe)
		return;
	for(auto v : pe->elements)
		add_menu(v, true);
	last_result = dialogv(0, format, 0);
	apply_result();
}

static bool talk_monsters() {
	if(!opponent)
		return false;
	auto pm = opponent->getmonster();
	if(!pm)
		return false;
	auto pq = party.getquest();
	if(!pq)
		return false;
	auto pn = speech_get_na(pm->id, pq->id);
	auto rm = bsdata<reactioni>::elements[last_reaction].id;
	if(!pn)
		pn = speech_get_na(pm->id, rm);
	if(!pn) {
		if(opponent->isanimal())
			pn = speech_get_na("Animal", rm);
		else
			pn = speech_get_na("Intellegence", rm);
	}
	if(!pn)
		return false;
	picture.clear();
	fix_animate();
	animation_update();
	talk_monsters(pn);
	return false;
}

static void party_set(creaturei** creatures, featn v) {
	for(auto i = 0; i < 6; i++) {
		if(creatures[i])
			creatures[i]->set(v);
	}
}

static void party_set(creaturei** creatures, directions v) {
	for(auto i = 0; i < 6; i++) {
		if(creatures[i])
			creatures[i]->d = v;
	}
}

void reaction_check(int bonus) {
	if(!loc)
		return;
	bonus = get_bonus(bonus);
	auto push_opponent = opponent;
	creaturei* creatures[6] = {};
	while(true) {
		loc->getmonsters(creatures, to(party, party.d));
		check_reaction(creatures, bonus);
		opponent = get_leader(creatures);
		if(!opponent)
			break;
		last_reaction = opponent->reaction;
		auto prev_reaction = last_reaction;
		party_set(creatures, Moved);
		party_set(creatures, to(party.d, Down));
		switch(last_reaction) {
		case Careful:
			talk_monsters();
			loc->getmonsters(creatures, to(party, party.d));
			break;
		case Friendly:
			talk_monsters();
			loc->getmonsters(creatures, to(party, party.d));
			break;
		case Indifferent:
			break;
		default:
			make_attacks(true);
			loc->getmonsters(creatures, to(party, party.d));
			break;
		}
		if(prev_reaction != last_reaction) {
			set_reaction(creatures, last_reaction);
			continue;
		}
		break;
	}
	opponent = push_opponent;
}

void move_party(pointc v) {
	if(!is_passable(v))
		return;
	if(loc->ismonster(v)) {
		turnto(v, to(party.d, Down), true, -party_median(characters, Sneaky));
		reaction_check(0);
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
	choose_quest();
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
	script_run(last_quest->travel);
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

static void create_power(int bonus) {
	last_item->createpower(bonus, 100, 0);
}

static void item_power_spell(int bonus) {
	auto power = last_item->getpower();
	if(!power)
		return;
	if(power.iskind<spelli>())
		last_spell = bsdata<spelli>::elements + power.value;
}

static void learn_last_spell(int bonus) {
	if(!last_spell)
		return;
	auto ps = get_spells_known(player);
	if(!ps)
		return;
	auto index = getbsi(last_spell);
	if(index == 0xFFFF)
		return;
	if(bonus >= 0)
		ps->set(index);
	else
		ps->remove(index);
}

static void learn_cleric_spells(int bonus) {
	auto ps = get_spells_known(player);
	if(!ps)
		return;
	pushanswer push;
	add_spells(0, bonus, 0);
	for(auto& e : an.elements)
		ps->set(getbsi((spelli*)e.value));
}

static void pay_gold(int bonus) {
	bonus = get_bonus(bonus);
	if(getparty(GoldPiece) < bonus) {
		dialog(0, speech_get(last_id, "NotEnoughtGold"), bonus);
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

static void dialog_message(const char* action) {
	auto pn = speech_get_na(last_id, action);
	if(pn)
		dialog(0, pn);
}

static void player_speak(const char* action) {
	player->speakn(last_id, action);
}

static void say_speech(int bonus) {
	switch(bonus) {
	case -1: player_speak("Fail"); break;
	default: player_speak("Success"); break;
	}
}

static void make_roll(int bonus) {
	if(!player->roll(last_ability, bonus * 5)) {
		script_stop();
		dialog_message("Fail");
		player_speak("FailSpeech");
		apply_script(last_id, "Fail", 0);
	} else {
		dialog_message("Success");
		player_speak("SuccessSpeech");
	}
}

static void set_level(int bonus) {
	last_number = last_level + get_bonus(bonus);
}

static void set_variable(int bonus) {
	party.abilities[last_variable] = get_bonus(bonus);
}

static void add_item(int bonus) {
	if(last_item)
		player->additem(*last_item);
}

static void add_reward(int bonus) {
	party.abilities[GoldPiece] += get_bonus(bonus) * 100;
	party_addexp(bonus * 200);
}

static void add_exp_group(int bonus) {
	party_addexp(bonus * 100);
}

static void add_exp_personal(int bonus) {
	player->addexp(bonus * 100);
}

static void add_exp_evil(int bonus) {
	party_addexp(LawfulEvil, bonus * 20);
	party_addexp(NeutralEvil, bonus * 20);
	party_addexp(ChaoticEvil, bonus * 20);
}

static void add_exp_good(int bonus) {
	party_addexp(LawfulGood, bonus * 30);
	party_addexp(NeutralGood, bonus * 20);
	party_addexp(ChaoticGood, bonus * 20);
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

static void push_player(int bonus) {
	auto push = player;
	script_run();
	player = push;
}

static void push_modifier(int bonus) {
	auto push = modifier;
	script_run();
	modifier = push;
}

static void push_item(int bonus) {
	auto push = last_item;
	script_run();
	last_item = push;
}

static void monsters_reaction(int bonus) {
	if(!loc)
		return;
	creaturei* creatures[6]; loc->getmonsters(creatures, to(party, party.d));
	for(auto p : creatures) {
		if(p)
			p->reaction = last_reaction;
	}
}

static void monsters_flee(int bonus) {
	if(!loc)
		return;
	pointca points;
	loc->block(false);
	loc->makewave(party);
	points.select(8, 16);
	if(!points)
		return;
	auto v = points.random();
	creaturei* creatures[6]; loc->getmonsters(creatures, to(party, party.d));
	for(auto p : creatures) {
		if(p) {
			p->x = v.x;
			p->y = v.y;
			p->set(Moved);
		}
	}
}

static void monsters_kill(int bonus) {
	if(!loc)
		return;
	creaturei* creatures[6]; loc->getmonsters(creatures, to(party, party.d));
	for(auto p : creatures) {
		if(p && *p)
			p->kill();
	}
}

static void monsters_leave(int bonus) {
	if(!loc)
		return;
	creaturei* creatures[6]; loc->getmonsters(creatures, to(party, party.d));
	for(auto p : creatures) {
		if(p && *p) {
			drop_unique_loot(p);
			loc->state.monsters_killed++;
			p->clear();
		}
	}
}

static void apply_switch(int bonus) {
	auto p1 = bsdata<listi>::find(str("%1Case%2i", last_id, last_number));
	if(p1)
		script_run(p1->id, p1->elements);
}

static void turning_monsters(int bonus) {
	// -2 value is automatic dispell and additional 2d4 turned (actually no work)
	// -1 value is automatic dispell
	// 0 value is automatic turned
	// 30 value is impossible turned
	static char chances[][14] = {
		{10, 7, 4, 0, 0, -1, -1, -2, -2, -2, -2, -2, -2, -2}, // 0 HD
		{13, 10, 7, 4, 0, 0, -1, -1, -2, -2, -2, -2, -2, -2}, // 1 HD
		{16, 13, 10, 7, 4, 0, 0, -1, -1, -2, -2, -2, -2, -2}, // 2 HD
		{19, 16, 13, 10, 7, 4, 0, 0, -1, -1, -2, -2, -2, -2}, // 3 HD
		{19, 16, 13, 10, 7, 4, 0, 0, -1, -1, -2, -2, -2, -2}, // 4 HD
		{20, 19, 16, 13, 10, 7, 4, 0, 0, -1, -1, -2, -2, -2}, // 5 HD
		{30, 20, 19, 16, 13, 10, 7, 4, 0, 0, -1, -1, -2, -2}, // 6 HD
		{30, 30, 20, 19, 16, 13, 10, 7, 4, 0, 0, -1, -1, -2}, // 7 HD
		{30, 30, 30, 20, 19, 16, 13, 10, 7, 4, 0, 0, -1, -1}, // 8 HD
		{30, 30, 30, 30, 20, 19, 16, 13, 10, 7, 4, 0, 0, -1}, // 9 HD
		{30, 30, 30, 30, 30, 20, 19, 16, 13, 10, 7, 4, 0, 0}, // 10 HD
		{30, 30, 30, 30, 30, 30, 20, 19, 16, 13, 10, 7, 4, 0}, // 11 HD
	};
	auto v = to(party, party.d);
	creaturei* creatures[6]; loc->getmonsters(creatures, v);
	auto pl = player->getlevel() + player->get(TurnUndeadBonus) + bonus;
	if(pl <= 0)
		return;
	if(pl > 14)
		pl = 14;
	for(auto p : creatures) {
		if(!p || p->isdisabled())
			continue;
		auto hd = p->getlevel();
		if(hd > 11)
			hd = 11;
		auto chance = chances[hd][pl - 1];
		if(chance < 0) {
			consolen("%1 turned %2 to dust", player->getname(), p->getname());
			monsters_kill(0);
		} else if(chance > 20) {
			consolen("%1 can't turn this creatures!", player->getname());
		} else if(chance == 0 || (d20() >= chance)) {
			consolen("%1 turned %2 to flee", player->getname(), p->getname());
			monsters_flee(0);
		} else
			consolen("%1 fail to turn %2", player->getname(), p->getname());
		break;
	}
}

static void empthy_script(int bonus) {
}

static void script_message(int bonus) {
	dialog_message("Message");
}

static void best_player(int bonus) {
	party_best(characters, last_ability, true);
	set_focus_by_player();
}

static void select_area(int bonus) {
	points.clear();
	if(!loc)
		return;
	if(!bonus)
		bonus = mpx;
	pointc v;
	for(v.y = 0; v.y < mpy; v.y++) {
		for(v.x = 0; v.x < mpx; v.x++) {
			switch(loc->get(v)) {
			case CellPassable:
			case CellWall:
			case CellUnknown:
				break;
			default:
				points.add(v);
				break;
			}
		}
	}
}

static bool filter_variant(variant v, variant t) {
	if(v.type == t.type)
		return v.value == t.value;
	else if(v.iskind<listi>()) {
		for(auto e : bsdata<listi>::elements[v.value].elements) {
			if(filter_variant(e, t))
				return true;
		}
	} else if(v.iskind<randomizeri>()) {
		for(auto e : bsdata<randomizeri>::elements[v.value].chance) {
			if(filter_variant(e, t))
				return true;
		}
	}
	return false;
}

static void clear_area(int bonus) {
	points.clear();
}

static void add_area_overlay(int bonus) {
	auto filter = next_script();
	auto keep = bonus >= 0;
	for(auto& e : loc->overlays) {
		if(!e.type)
			continue;
		variant t = bsdata<celli>::elements + e.type;
		if(filter_variant(filter, t) != keep)
			continue;
		auto v = to(e, e.d);
		points.addu(v);
	}
}

static void add_area_items(int bonus) {
	auto filter = next_script();
	auto keep = bonus >= 0;
	// Ground items
	for(auto& e : loc->items) {
		if(!e)
			continue;
		if(filter_variant(filter, &e.geti()) != keep)
			continue;
		auto v = to(e, e.d);
		points.addu(v);
	}
	// Wall cellar items
	for(auto& e : loc->overlayitems) {
		if(!e)
			continue;
		if(filter_variant(filter, &e.geti()) != keep)
			continue;
		auto v = loc->overlays[e.storage_index];
		points.addu(v);
	}
}

static void add_area_monsters(int bonus) {
	for(auto& e : loc->monsters) {
		if(!e || e.isdisabled())
			continue;
		points.addu(e);
	}
}

static void filter_area(int bonus) {
	auto filter = next_script();
	auto ps = points.begin();
	auto pe = points.end();
	auto keep = bonus >= 0;
	auto push_point = last_point;
	for(auto pb = points.begin(); pb < pe; pb++) {
		last_point = *pb;
		variant v = bsdata<celli>::elements + loc->get(last_point);
		if(filter_variant(filter, v) != keep)
			continue;
		*ps++ = *pb;
	}
	last_point = push_point;
	points.count = ps - points.begin();
}

static void filter_area(cellfn flag, bool keep) {
	auto ps = points.begin();
	auto pe = points.end();
	auto push_point = last_point;
	for(auto pb = points.begin(); pb < pe; pb++) {
		last_point = *pb;
		if(loc->is(last_point, flag) != keep)
			continue;
		*ps++ = *pb;
	}
	last_point = push_point;
	points.count = ps - points.begin();
}

static void filter_area_explored(int bonus) {
	filter_area(CellExplored, bonus >= 0);
}

static void random_area(int bonus) {
	if(bonus <= 1)
		bonus = 1;
	points.random(bonus);
}

static void show_area(int bonus) {
	if(!bonus)
		bonus = 1;
	if(points) {
		show_automap(points, bonus);
		say_speech(0);
	} else
		say_speech(-1);
}

static void pass_round(int bonus) {
	pass_round();
}

static void choose_shop_item(int bonus) {
	// Function use huge amount of memory for existing copy of answers and return result to upper level.
	// So this memory used only for selection, not for each level of hierarhi.
	pushanswer push;
	char header[64]; stringbuilder sb(header);
	set_player_by_focus();
	sb.add(get_header(last_shop->id, "Options"));
	for(auto& v : last_shop->items) {
		if(!v)
			continue;
		switch(bonus) {
		case 1: an.add(&v, v.getpower().getname()); break;
		default: an.add(&v, v.getname()); break;
		}
	}
	last_item = (item*)choose_large_menu(header, getnm("Cancel"));
	if(!last_item)
		script_stop();
}

static bool use_bless_effect(const variants& source, int bonus) {
	auto level = player->getlevel() + bonus;
	for(auto v : source) {
		if(v.iskind<spelli>()) {
			auto p = bsdata<spelli>::elements + v.value;
			if(!cast_spell(p, level, 0, false, false, 0))
				continue;
			cast_spell(p, level, 0, true, true, 0);
			return true;
		}
	}
	return false;
}

static void consume_tool(int chance, int count, const char* message_id = 0) {
	if(d100() < chance) {
		auto tool_id = last_item->geti().id;
		last_item->damage(count);
		if(!(*last_item)) {
			if(!message_id)
				message_id = "ToolBroken";
			consolen(getnm(message_id), getnm(tool_id));
		}
	}
}

static void use_holy_symbol(int bonus) {
	auto pi = bsdata<listi>::find("GoodDietyDomain");
	if(!pi)
		return;
	if(!bonus)
		bonus = 4;
	if(use_bless_effect(pi->elements, bonus)) {
		if(result_player)
			consolen(getnm("UseHolySymbolSuccessOnPlayer"), "Helm", result_player->getname());
		else
			consolen(getnm("UseHolySymbolSuccess"), "Helm");
	} else
		consolen(getnm("UseHolySymbolFail"));
	consume_tool(60, xrand(1, 3), "ToolCrumbleToDust");
	pass_round();
}

static void player_name(stringbuilder& sb) {
	sb.add(player->getname());
}

static void opponent_name(stringbuilder& sb) {
	sb.add(opponent->getname());
}

static void item_name(stringbuilder& sb) {
	last_item->getname(sb);
}

static void spell_name(stringbuilder& sb) {
	if(last_spell)
		sb.add(last_spell->getname());
	else
		sb.add("unknown spell");
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

static void dungeon_special(stringbuilder& sb) {
	if(loc->special)
		sb.addv(bsdata<itemi>::elements[loc->special].getname(), 0);
}

static void dungeon_boss(stringbuilder& sb) {
	if(loc && loc->boss)
		sb.addv(bsdata<monsteri>::elements[loc->boss].getname(), 0);
}

static void dungeon_key(stringbuilder& sb) {
	sb.addv(getnm(loc->getkey()->id), 0);
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

static bool if_area_locked() {
	auto po = loc->get(*player, player->d);
	if(!po || po->type != CellKeyHole || !po->link)
		return false;
	return !loc->is(po->link, CellActive);
}

static bool if_diseased() {
	return player->is(DiseaseLevel);
}

static bool if_intelligence() {
	return player->get(Intellegence) >= 6;
}

static bool if_poisoned() {
	return player->is(PoisonLevel);
}

static bool if_paralized() {
	return player->is(Paralized);
}

static bool if_party(conditioni::fntest proc) {
	auto push = player;
	for(auto p : characters) {
		if(!p || !p->isready())
			continue;
		player = p;
		if(proc()) {
			player = push;
			return true;
		}
	}
	player = push;
	return false;
}

static bool if_party_item(conditioni::fntest proc) {
	auto push = last_item;
	for(auto& e : player->wears) {
		if(!e)
			continue;
		last_item = &e;
		if(proc()) {
			last_item = push;
			return true;
		}
	}
	last_item = push;
	return false;
}

static bool if_party_item(conditioni::fntest player_test, conditioni::fntest proc) {
	auto push_player = player;
	for(auto p : characters) {
		if(!p || !p->isready())
			continue;
		player = p;
		if(player_test && !player_test())
			continue;
		if(if_party_item(proc)) {
			player = push_player;
			return true;
		}
	}
	player = push_player;
	return false;
}

static bool if_wounded() {
	return player->gethp() < player->hpm;
}

static bool if_prepared() {
	auto v = next_script();
	if(v.iskind<spelli>())
		return player->spells[v.value] > 0;
	return false;
}

static bool if_monsters(conditioni::fntest test) {
	creaturei* creatures[6]; loc->getmonsters(creatures, to(party, party.d));
	auto push_player = player;
	for(auto p : creatures) {
		if(!p || p->isdisabled())
			continue;
		player = p;
		if(test()) {
			player = push_player;
			return true;
		}
	}
	player = push_player;
	return false;
}

static bool if_undead() {
	return player->is(Undead);
}

static bool if_monsters_undead() {
	return if_monsters(if_undead);
}

static bool if_monsters_intellegence() {
	return if_monsters(if_intelligence);
}

static bool if_item_damaged() {
	return last_item->isdamaged();
}

static bool if_item_bribe() {
	if(last_item->isidentified() && last_item->ismagical())
		return false;
	auto& ei = last_item->geti();
	if(ei.wear == LeftRing || ei.wear == RightRing || ei.wear == Neck)
		return true;
	return ei.wear == Backpack && ei.cost >= 150;
}

static bool if_item_edible() {
	return last_item->geti().wear == Edible;
}

static bool if_item_identified() {
	return last_item->isidentified();
}

static bool if_item_cursed() {
	return last_item->iscursed();
}

static bool if_item_magic() {
	return last_item->getpower().counter != 0;
}

static bool if_item_readable() {
	return last_item->geti().wear == Readable;
}

static bool if_item_rod() {
	return last_item->geti().wear == Rod;
}

static bool if_last_item() {
	return last_item != 0;
}

static bool if_low_level() {
	return player->getlevel() <= 4;
}

static bool if_item_known_spell() {
	auto power = last_item->getpower();
	if(power.iskind<spelli>()) {
		auto ps = get_spells_known(player);
		if(!ps)
			return false;
		return ps->is(power.value);
	}
	return false;
}

static bool if_item_charged() {
	return last_item->geti().wear == Rod;
}

static void talk_standart() {
	dialog(0, speech_get(last_talk->id), 0);
}

static bool talk_stage(bool run) {
	auto pq = party.getquest();
	if(!pq)
		return false;
	auto new_stage = party.stages[party.quest_id] + 1;
	auto pn = speech_get(str("%1Stage%2i", pq->id, new_stage));
	if(!pn)
		return false;
	if(run) {
		dialog(0, pn);
		party.stages[party.quest_id]++;
		party_addexp(200);
	}
	return true;
}

static bool talk_cursed(bool run) {
	pushanswer push;
	select_items(if_item_identified, false);
	filter_items(if_item_cursed, true);
	if(!an)
		return false;
	if(run) {
		last_item = (item*)an.elements[rand() % an.getcount()].value;
		last_item->identify(1);
		talk_standart();
	}
	return true;
}

static bool talk_magical(bool run) {
	pushanswer push;
	select_items(if_item_identified, false);
	filter_items(if_item_magic, true);
	if(!an)
		return false;
	if(run) {
		last_item = (item*)an.elements[rand() % an.getcount()].value;
		last_item->identify(1);
		talk_standart();
	}
	return true;
}

static bool talk_key(bool run) {
	if(run) {
		item it; it.create(loc->key);
		it.identify(1);
		player->additem(it);
		talk_standart();
	}
	return true;
}

static bool talk_gift(bool run) {
	auto pi = bsdata<randomizeri>::find("MonsterGiftItems");
	if(!pi)
		return false;
	if(run) {
		auto v = single(pi);
		if(v.iskind<itemi>()) {
			item it;
			it.create(v.value);
			it.createpower(2, 20, 30);
			it.identify(-1);
			player->additem(it);
			talk_standart();
		}
	}
	return true;
}

static bool talk_about_proc(bool run) {
	adat<talking*> source;
	auto push = last_talk;
	for(auto& e : bsdata<talking>()) {
		last_talk = &e;
		if(!e.proc(false))
			continue;
		source.add(&e);
	}
	last_talk = push;
	if(!source)
		return false;
	if(run) {
		// Take first with greater chance.
		last_talk = (d100() < 40) ? source[0] : source.random();
		last_talk->proc(true);
	}
	last_talk = push;
	return true;
}

static bool if_talk() {
	return talk_about_proc(false);
}

static void talk_about(int bonus) {
	talk_about_proc(true);
}

BSDATA(textscript) = {
	{"DungeonBoss", dungeon_boss},
	{"DungeonKey", dungeon_key},
	{"DungeonOrigin", dungeon_origin},
	{"DungeonSpecial", dungeon_special},
	{"Habbitant1", dungeon_habbitant1},
	{"Habbitant2", dungeon_habbitant2},
	{"ItemName", item_name},
	{"Name", player_name},
	{"OpponentName", opponent_name},
	{"Number", effect_number},
	{"StairsDownSide", stairs_down_side},
	{"StairsUpSide", stairs_up_side},
	{"SpellName", spell_name},
};
BSDATAF(textscript)
BSDATA(talking) = {
	{"TalkStage", talk_stage},
	{"TalkCursed", talk_cursed},
	{"TalkMagical", talk_magical},
	{"TalkGift", talk_gift},
	{"TalkKey", talk_key},
};
BSDATAF(talking)
BSDATA(conditioni) = {
	{"IfAlive", if_alive},
	{"IfAreaLocked", if_area_locked},
	{"IfDiseased", if_diseased},
	{"IfIntelligence", if_intelligence},
	{"IfItemBribe", if_item_bribe},
	{"IfItemCharged", if_item_charged},
	{"IfItemCursed", if_item_cursed},
	{"IfItemDamaged", if_item_damaged},
	{"IfItemEdible", if_item_edible},
	{"IfItemIdentified", if_item_identified},
	{"IfItemMagic", if_item_magic},
	{"IfItemReadable", if_item_readable},
	{"IfItemRod", if_item_rod},
	{"IfItemKnownSpell", if_item_known_spell},
	{"IfLastItem", if_last_item},
	{"IfLowLevel", if_low_level},
	{"IfMonstersUndead", if_monsters_undead},
	{"IfParalized", if_paralized},
	{"IfPoisoned", if_poisoned},
	{"IfPrepared", if_prepared},
	{"IfTalk", if_talk},
	{"IfWounded", if_wounded},
};
BSDATAF(conditioni)
BSDATA(script) = {
	{"AllLanguages", all_languages},
	{"Attack", attack_modify},
	{"ActionItems", action_items},
	{"ActionPlayerItems", action_player_items},
	{"ActivateQuest", activate_quest},
	{"ActivateLinkedOverlay", activate_linked_overlay},
	{"AddAid", player_add_aid},
	{"AddAreaItems", add_area_items},
	{"AddAreaMonsters", add_area_monsters},
	{"AddAreaOverlay", add_area_overlay},
	{"AddExp", add_exp_group},
	{"AddExpPersonal", add_exp_personal},
	{"AddExpEvil", add_exp_evil},
	{"AddExpGood", add_exp_good},
	{"AddItem", add_item},
	{"AddReward", add_reward},
	{"AddVariable", add_variable},
	{"ApplyAction", apply_action},
	{"ApplyEnchantSpell", apply_enchant_spell},
	{"ApplyRacialEnemy", apply_racial_enemy},
	{"BestPlayer", best_player},
	{"BuyMenu", buy_menu},
	{"ConfirmAction", confirm_action},
	{"Character", set_character},
	{"ChooseItems", choose_items},
	{"ChooseMenu", choose_menu},
	{"ChooseShopItem", choose_shop_item},
	{"ChooseSpells", choose_spells},
	{"ClearArea", clear_area},
	{"CreateCharacter", create_character},
	{"CreatePower", create_power},
	{"CurseItem", curse_item},
	{"Damage", damage_modify},
	{"DamageItem", damage_item},
	{"DestroyItem", destroy_item},
	{"DoneQuest", done_quest},
	{"ExitGame", exit_game},
	{"EnterDungeon", enter_dungeon},
	{"EnterLocation", enter_location},
	{"FilterArea", filter_area},
	{"FilterAreaExplored", filter_area_explored},
	{"ForEachItem", for_each_item},
	{"ForEachParty", for_each_party},
	{"Heal", player_heal},
	{"HealEffect", player_heal_effect},
	{"IdentifyItem", identify_item},
	{"ItemPowerSpell", item_power_spell},
	{"LearnClericSpells", learn_cleric_spells},
	{"LearnLastSpell", learn_last_spell},
	{"LoadGame", load_game},
	{"Magical", empthy_script},
	{"Message", script_message},
	{"MonstersFlee", monsters_flee},
	{"MonstersKill", monsters_kill},
	{"MonstersLeave", monsters_leave},
	{"MonstersReaction", monsters_reaction},
	{"NaturalHeal", natural_heal},
	{"JoinParty", join_party},
	{"PartyAdventure", party_adventure},
	{"PayGold", pay_gold},
	{"PassHours", pass_hours},
	{"PassRound", pass_round},
	{"Protection", protection_modify},
	{"PushItem", push_item},
	{"PushModifier", push_modifier},
	{"PushPlayer", push_player},
	{"RandomArea", random_area},
	{"ReactionCheck", reaction_check},
	{"RestoreSpells", restore_spells},
	{"ReturnToStreet", return_to_street},
	{"Roll", make_roll},
	{"Satisfy", satisfy},
	{"SaySpeech", say_speech},
	{"SaveGame", save_game},
	{"Saves", saves_modify},
	{"SaveHalf", save_half},
	{"SaveNegate", save_negate},
	{"SelectArea", select_area},
	{"SetLevel", set_level},
	{"SetVariable", set_variable},
	{"SleepParty", sleep_party},
	{"ShowArea", show_area},
	{"Switch", apply_switch},
	{"TalkAbout", talk_about},
	{"TurningMonsters", turning_monsters},
	{"UseHolySymbolEvil", use_holy_symbol},
	{"UseTheifTools", use_theif_tools},
	{"Wizardy", wizardy_effect},
};
BSDATAF(script)