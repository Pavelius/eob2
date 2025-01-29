#include "action.h"
#include "alignment.h"
#include "answers.h"
#include "avatar.h"
#include "boost.h"
#include "carousing.h"
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
	case 100: return player->get(last_ability);
	case 101: return last_number;
	case 102: return last_level;
	case -100: return -player->get(last_ability);
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

template<> bool fttest<genderi>(int value, int bonus) {
	return (player->gender == (gendern)value) == (bonus >= 0);
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
	auto p = bsdata<quest>::elements + value;
	if(bonus >= 0) {
		p->prepare();
		p->stage = quest::Active;
	} else
		p->stage = quest::Hide;
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
	case Permanent: player->basic.abilities[value] += get_bonus(bonus); break;
	default: player->add((abilityn)value, get_bonus(bonus)); break;
	}
}

template<> void ftscript<partystati>(int value, int bonus) {
	add_party((partystatn)value, get_bonus(bonus));
}

template<> void ftscript<itemi>(int value, int bonus) {
	item v; v.create(value);
	v.createpower(bonus, bonus ? 100 : 0, 0);
	switch(modifier) {
	case Grounding:
		if(loc && last_point)
			loc->drop(last_point, v, xrand(0, 3));
		break;
	default:
		player->additem(v);
		break;
	}
}

static dungeoni* find_dungeon(int level) {
	for(auto& e : last_quest->dungeon) {
		if(e.level == level)
			return &e;
	}
	return 0;
}

static int get_maximum_stage() {
	if(!last_quest)
		return 0;
	auto result = 0;
	while(getnme(str("%1Stage%2i", last_quest->id, result + 1)) != 0)
		result++;
	return result;
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
	case 4: ftscript<abilityi>(Spell4, player->basic.abilities[Spell4]); break;
	case 5: ftscript<abilityi>(Spell5, player->basic.abilities[Spell5]); break;
	case -2: ftscript<abilityi>(Spell1, -4); ftscript<abilityi>(Spell2, -4); break; // Cursed version
	default: break;
	}
}

static bool apply_script(const char* id, const char* action, const char* postfix, int bonus) {
	pushvalue push(last_id);
	auto sid = ids(id, action, postfix);
	auto p1 = bsdata<script>::find(sid);
	if(p1) {
		last_id = p1->id;
		p1->proc(bonus);
		return true;
	}
	auto p2 = bsdata<listi>::find(sid);
	if(p2) {
		last_id = p2->id;
		script_run(p2->elements);
		return true;
	}
	auto p3 = bsdata<randomizeri>::find(sid);
	if(p3) {
		last_id = p3->id;
		auto v = single(p3->random());
		v.counter = bonus;
		script_run(v);
		return true;
	}
	return false;
}

static bool apply_script(const char* id, const char* action, int bonus) {
	if(last_quest) {
		if(apply_script(id, action, last_quest->id, bonus))
			return true;
	}
	return apply_script(id, action, 0, bonus);
}

static const char* find_speak(const char* id, const char* action) {
	if(!player)
		return 0;
	if(last_quest) {
		auto p = speech_get(ids(id, action, last_quest->id));
		if(p)
			return p;
	}
	auto& ei = player->getclass();
	for(auto i = 0; i < ei.count; i++) {
		auto p = speech_get(ids(id, action, getid<classi>(ei.classes[i])));
		if(p)
			return p;
	}
	auto p = speech_get(ids(id, action, player->getrace().id));
	if(p)
		return p;
	return speech_get(ids(id, action, 0));
}

static const char* find_text(const char* id, const char* action) {
	if(!player)
		return 0;
	if(last_quest) {
		auto p = getnme(ids(id, action, last_quest->id));
		if(p)
			return p;
	}
	auto& ei = player->getclass();
	for(auto i = 0; i < ei.count; i++) {
		auto p = getnme(ids(id, action, getid<classi>(ei.classes[i])));
		if(p)
			return p;
	}
	auto p = getnme(ids(id, action, player->getrace().id));
	if(p)
		return p;
	return getnme(ids(id, action, 0));
}

static void dialog_message(const char* action) {
	auto format = find_speak(last_id, action);
	if(!format)
		format = find_text(last_id, action);
	if(format)
		dialog(0, format);
}

bool apply_message(const char* id, const char* action) {
	if(!player)
		return false;
	auto format = find_speak(id, action);
	if(format) {
		player->sayv(format, 0);
		return true;
	}
	format = find_text(id, action);
	if(format) {
		consolenl();
		consolev(format, 0);
		return true;
	}
	return false;
}

bool apply_action_dialog(const char* id, const char* action) {
	if(!player)
		return false;
	auto format = find_speak(id, action);
	if(!format)
		format = find_text(id, action);
	if(!format)
		return false;
	dialog(0, format);
	return false;
}

void broke_cell(pointc v) {
	auto t = loc->get(v);
	auto broken_cell = bsdata<celli>::elements[t].activate;
	if(!broken_cell)
		broken_cell = CellPassable;
	loc->set(v, broken_cell);
	animation_update();
	pushvalue push_point(last_point, v);
	pushvalue push_modifier(modifier, Grounding);
	if(apply_script(bsdata<celli>::elements[t].id, "Use", 0))
		apply_message(bsdata<celli>::elements[t].id, "Use");
}

static bool if_monster_nearbe(pointc v) {
	if(!pathmap[v.y][v.x] || pathmap[v.y][v.x] > 5)
		return false;
	return loc->ismonster(v);
}

static bool monsters_nearbe() {
	loc->block(false);
	loc->makewave(party);
	auto result = loc->is(if_monster_nearbe);
	if(result)
		apply_message("MonstersNearbe", "Fail");
	return result;
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
		if(p->restrict_classes && party_have_class(p->restrict_classes))
			return;
		if(!ismatch(p->required))
			return;
		if(whole_party) {
			if(p->classes && !party_have_class(p->classes))
				return;
			if(p->races && !party_have_race(p->classes))
				return;
			if(p->alignment && !party_have_alignment(p->alignment))
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

static bool choose_dialog(const char* format, const char* format_param, const char* yes, const char* no) {
	pushanswer push;
	an.add((void*)1, yes);
	return (bool)dialogv(no, format, format_param);
}

static bool confirm_payment(const char* name, int gold_coins) {
	pushvalue push_number(last_number, gold_coins);
	char temp[260]; stringbuilder sb(temp);
	auto format = getnme(ids(last_id, "Confirm"));
	if(!format)
		format = getnm("PayGoldConfirm");
	sb.add(format, name, gold_coins, party.abilities[GoldPiece]);
	if(getparty(GoldPiece) >= gold_coins) {
		if(!choose_dialog(temp, 0, getnm("Accept"), getnm("Decline")))
			return false;
	} else {
		pushanswer an;
		dialogv(getnm("Decline"), temp, 0);
		return false;
	}
	add_party(GoldPiece, -gold_coins);
	return true;
}

static bool apply_cost(const char* id, int cost, const picturei& avatar) {
	if(!cost)
		return true;
	pushvalue push_id(last_id, id);
	pushvalue push_picture(picture);
	if(avatar)
		picture = avatar;
	return confirm_payment(getnm(id), cost);
}

static void apply_action(int bonus) {
	if(!apply_cost(last_action->id, last_action->cost, last_action->avatar))
		return;
	if(last_action->avatar)
		picture = last_action->avatar;
	script_run(last_action->id, last_action->effect);
}

static void apply_result() {
	if(!last_result) {
		last_number = 0;
	} else if(bsdata<locationi>::have(last_result)) {
		auto p = (locationi*)last_result;
		if(!apply_cost(p->id, p->cost, p->avatar))
			return;
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

static void choose_item(const char* id, const variants& filter, int bonus) {
	pushanswer push;
	char header[64]; stringbuilder sb(header);
	set_player_by_focus();
	sb.add(get_header(id, "Options"));
	for(auto& e : player->wears) {
		last_item = &e;
		if(!script_allow(filter))
			continue;
		switch(bonus) {
		case 1: an.add(&e, e.getpower().getname()); break;
		default: an.add(&e, e.getname()); break;
		}
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
	auto cost = pi->getcost() / 2; // Selling is half priced by default
	if(!cost)
		return;
	if(!can_remove(pi))
		return;
	if(!can_loose(pi))
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

static void update_camp_spells(variant v) {
	if(!v)
		return;
	if(v.iskind<spelli>())
		player->spells[v.value] += v.counter;
	else if(v.iskind<listi>()) {
		for(auto e : bsdata<listi>::elements[v.value].elements)
			update_camp_spells(e);
	}
}

static void update_camp_spells() {
	for(auto& e : player->equipment()) {
		if(!e)
			continue;
		auto w = e.geti().wear;
		if(w == RightHand || (w >= Backpack && w <= LastBackpack))
			continue;
		update_camp_spells(e.getpower());
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
	camp_autocast();
	natural_heal(bonus);
	satisfy(0);
	restore_spells(0);
}

static void strenght_add(int bonus) {
	if(bonus > 0) {
		bonus += player->abilities[Strenght];
		if(bonus > 18) {
			player->abilities[Strenght] = 18;
			bonus = player->abilities[ExeptionalStrenght] + (bonus - 18) * 10;
			if(bonus > 100)
				bonus = 100;
			player->abilities[ExeptionalStrenght] = bonus;
		} else
			player->abilities[Strenght] = bonus;
	}
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
			return 4 - player->basic.abilities[a];
		return 17 + magical_bonus - player->basic.abilities[a];
	case ExeptionalStrenght:
		return magical_bonus * 10;
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
		case 0: return 30;
		case 1: return 60;
		case 2: case 3: case 4: case 5: return 90;
		default: return magical_bonus;
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
		if(v.value >= AcidD1Level)
			player->add((abilityn)v.value, v.counter * multiplier);
		else if(v.counter >= permanent)
			player->basic.add((abilityn)v.value, (v.counter - permanent + 1) * multiplier);
		else {
			if(v.value == Strenght && player->get(Strenght) == 18)
				v.value = ExeptionalStrenght;
			v.counter = get_ability_number(player, (abilityn)v.value, v.counter * multiplier);
			if(v.counter)
				add_boost(party.abilities[Minutes] + duration, player, v.value, v.counter);
		}
	} else if(v.iskind<feati>()) {
		v.counter = multiplier;
		add_boost(party.abilities[Minutes] + duration, player, BoostFeat, v.value);
	}
}

static bool read_effect(creaturei* pn, variant v, int experience, unsigned duration) {
	bool result = false;
	last_number = duration;
	auto push_player = player; player = pn;
	if(v.iskind<spelli>()) {
		result = cast_spell(bsdata<spelli>::elements + v.value, player->getlevel() + v.counter, experience, true, false, 0, 0);
		if(result)
			pass_round();
	} else if(v.iskind<listi>() || v.iskind<randomizeri>() || v.iskind<script>()) {
		if(!player->roll(Intellegence)) {
			auto format = find_speak("LearnTome", "Fail");
			if(format) {
				consolenl();
				consolev(format, 0);
			}
			result = false;
		} else
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
		result = cast_spell(ps, 1 + v.counter * 2, 0, true, false, 0, 0);
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

static bool use_bless_effect(const variants& source, int bonus, bool run) {
	auto level = player->getlevel() + bonus;
	auto duration = (4 + bonus) * 60; // 4-9 hour standart duration for blessing
	for(auto v : source) {
		if(v.iskind<spelli>()) {
			last_spell = bsdata<spelli>::elements + v.value;
			if(!cast_spell(last_spell, level, 0, false, false, duration, 0))
				continue;
			if(run)
				cast_spell(last_spell, level, 0, true, true, duration, 0);
			return true;
		}
	}
	return false;
}

static bool faith_effect(int bonus) {
	auto powers = bsdata<listi>::find("GoodDietyPowers");
	if(!powers)
		return false;
	if(party.abilities[Blessing] <= 30) {
		consolen(getnm("YourFaithIsWeak"));
		return false;
	}
	pushvalue push_spell(last_spell);
	if(!use_bless_effect(powers->elements, bonus, false)) {
		consolen(getnm("UseHolySymbolNoEffect"));
		return false;
	}
	if(d100() >= party.abilities[Blessing]) {
		consolen(getnm("UseHolySymbolFail"));
		return true;
	}
	if(use_bless_effect(powers->elements, bonus, true)) {
		if(result_player)
			consolen(getnm("UseHolySymbolSuccessOnPlayer"), result_player->getname());
		else
			consolen(getnm("UseHolySymbolSuccess"));
		add_party(Blessing, -1);
	} else
		consolen(getnm("UseHolySymbolFail"));
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
			if(!make_object_attack(to(party, party.d)))
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
		if(monsters_nearbe())
			break;
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
		if(w != LeftHand) {
			pn->speak("MustBeWearing", "LeftHand");
			break;
		}
		if(use_rod(pn, last_item, last_item->getpower()))
			pass_round();
		break;
	case Faithable:
		if(!allow_use(pn, last_item))
			break;
		if(w != LeftHand) {
			pn->speak("MustBeWearing", "LeftHand");
			break;
		}
		if(faith_effect(last_item->getmagic())) {
			last_item->usecharge("ToolCrumbleToDust");
			pass_round();
		}
		break;
	case Usable:
		if(!allow_use(pn, last_item))
			break;
		apply_script(last_item->geti().id, "Use", last_item->getmagic());
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

#ifdef _DEBUG

static void explore_all_dungeon() {
	if(!loc)
		return;
	pointc v;
	for(v.x = 0; v.x < mpx; v.x++) {
		for(v.y = 0; v.y < mpy; v.y++) {
			if(v)
				loc->set(v, CellExplored);
		}
	}
}

static void test_throw_item() {
	if(!loc)
		return;
	pointc v = party;
	for(int i = 0; i < 3; i++)
		v = to(v, party.d);
	thrown_item(v, Down, 106, get_party_index(player) % 2, 4);
}

static void test_ground() {
	explore_all_dungeon();
	test_throw_item();
}

#endif // _DEBUG

static void choose_race(int bonus) {
	pushanswer push;
	for(auto& e : bsdata<racei>()) {
		if(!e.maximal[0])
			continue;
		an.add(&e, e.getname());
	}
	auto p = (racei*)choose_generate_dialog(getnm("ChooseRace"), bonus > 0);
	if(!p)
		return;
	last_race = (racen)(p - bsdata<racei>::elements);
}

static void choose_class(int bonus) {
	pushanswer push;
	for(auto& e : bsdata<classi>()) {
		if(e.non_player)
			continue;
		if(e.races && !e.races.is(last_race))
			continue;
		an.add(&e, e.getname());
	}
	auto p = (classi*)choose_generate_dialog(getnm("ChooseClass"), bonus > 0);
	if(!p)
		return;
	last_class = (classn)(p - bsdata<classi>::elements);
}

static void choose_alignment(int bonus) {
	pushanswer push;
	auto& ei = bsdata<classi>::elements[last_class];
	for(auto& e : bsdata<alignmenti>()) {
		if(ei.alignment && !ei.alignment.is(bsdata<alignmenti>::source.indexof(&e)))
			continue;
		an.add(&e, e.getname());
	}
	auto p = (alignmenti*)choose_generate_dialog(getnm("ChooseAlignment"), bonus > 0);
	if(!p)
		return;
	last_alignment = (alignmentn)(p - bsdata<alignmenti>::elements);
}

static void choose_gender(int bonus) {
	pushanswer push;
	an.add(bsdata<genderi>::elements + Male, getnm("Male"));
	an.add(bsdata<genderi>::elements + Female, getnm("Female"));
	auto p = (genderi*)choose_generate_dialog(getnm("ChooseGender"), bonus > 0);
	if(!p)
		return;
	last_gender = (gendern)(p - bsdata<genderi>::elements);
}

static creaturei** choose_player_position() {
	if(is_party_full())
		return (creaturei**)choose_generate_box(getnm("ChooseGenerateOptions"), getnm("ChooseGeneratePlay"));
	else
		return (creaturei**)choose_generate_box(getnm("ChooseGenerateOptions"), 0);
}

static void generate_party(int bonus) {
	while(true) {
		player_position = choose_player_position();
		if(!player_position)
			break;
		if(*player_position)
			choose_generate_box(paint_character_edit);
		else {
			choose_race(0);
			choose_gender(0);
			choose_class(0);
			choose_alignment(0);
			player = bsdata<creaturei>::addz();
			player->clear();
			clear_spellbook();
			create_npc(player, 0, is_party_name);
			generate_abilities();
			apply_race_ability();
			roll_player_hits();
			update_player();
			update_player_hits();
			if(!choose_avatar()) {
				player->clear();
				continue;
			}
			create_player_finish();
			*player_position = player;
			choose_generate_box(paint_character_edit);
		}
	}
	// Join party
	for(auto p : characters) {
		if(!p)
			continue;
		player = p;
		join_party();
	}
}

static void change_quick_item() {
	auto pi = (item*)current_focus;
	auto pn = item_owner(pi);
	auto w = item_wear(pi);
	change_quick_item(pn, w);
}

static void city_adventure_input() {
	static hotkeyi keys[] = {
		{KeyEscape, choose_city_menu},
		{'E', cast_spell},
		{'D', drop_city_item},
		{'U', use_item},
		{'R', change_quick_item},
#ifdef _DEBUG
		{'T', test_ground},
#endif
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

static bool is_special_item(const item& it) {
	if(!last_quest)
		return false;
	auto item_type = getbsi(&it.geti());
	if(!item_type)
		return false;
	for(auto& e : last_quest->sites) {
		if(!e || !e.special)
			break;
		if(e.special == item_type)
			return true;
	}
	return false;
}

static void clear_quest_items() {
	for(auto& e : player->wears) {
		if(e && is_special_item(e))
			e.clear();
	}
}

static void activate_next_quest() {
	for(auto& e : bsdata<quest>()) {
		if(!e)
			continue;
		if(e.stage != quest::Hide)
			continue;
		e.stage = quest::Active;
		break;
	}
}

static const char* get_stage_id(const quest* p, short unsigned monster_id, int stage, const char* action) {
	return str("%1%2%4%3i", p->id, bsdata<monsteri>::elements[monster_id].id, stage, action);
}

static const char* get_stage_text(const quest* p, short unsigned monster_id, int stage, const char* action) {
	return getnme(get_stage_id(p, monster_id, stage, action));
}

static void check_history_completed() {
	for(auto& q : bsdata<quest>()) {
		if(!q.dungeon)
			continue;
		for(auto& e : q.history) {
			if(e.stage >= e.value)
				continue;
			for(auto i = e.stage + 1; i <= e.value; i++) {
				auto pn = get_stage_text(&q, e.monster, i, "Explain");
				if(pn)
					message(pn);
				auto ps = bsdata<listi>::find(get_stage_id(&q, e.monster, i, "Explain"));
				if(ps)
					script_run(ps->id, ps->elements);
				e.stage = i;
			}
		}
	}
}

static void check_quest_complited() {
	if(!last_quest_complite())
		return;
	auto push_quest = last_quest;
	auto pn = getnme(ids(last_quest->id, "Finish"));
	if(pn)
		message(pn);
	all_party(clear_quest_items, false); // Remove quest item only if done quest
	script_run(last_quest->reward);
	last_quest = push_quest;
	last_quest->stage = quest::Done;
	activate_next_quest();
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

static void party_unlock(int bonus) {
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

static void enter_location() {
	loc = 0;
	party.location_id = getbsi(last_location);
	picture = last_location->avatar;
	save_focus = current_focus;
	set_next_scene(play_location);
}

static void enter_location(int bonus) {
	if(loc) {
		last_dungeon = loc;
		last_exit = party;
		all_party(clear_mission_equipment, false);
		all_party(clear_edible, false);
		check_history_completed();
		check_quest_complited();
		after_dungeon_action("LootIdentyfing", loot_identyfing);
		after_dungeon_action("LootSelling", loot_selling);
		pass_hours(xrand(2, 4));
		party_unlock(0);
	}
	last_quest = 0;
	enter_location();
}

static void enter_sanctuary(int bonus) {
	if(loc) {
		last_dungeon = loc;
		last_exit = party;
		pass_hours(1);
	}
	enter_location();
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
	choose_item(last_id, commands, bonus);
	script_stop();
}

static void for_each_party(int bonus, const variants& commands, const slice<creaturei*>& characters) {
	auto push_player = player;
	for(auto p : characters) {
		if(!p)
			continue;
		if(bonus == 0 && !p->isready())
			continue;
		if(bonus == -1 && p->isdead())
			continue;
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

static void for_each_opponents(int bonus) {
	scriptbody commands;
	creaturei* opponents[6]; loc->getmonsters(opponents, party, party.d);
	for_each_party(bonus, commands, opponents);
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

static void done_quest(int bonus) {
	if(last_quest)
		last_quest->stage = quest::Done;
}

static void choose_quest() {
	auto push_answers = an;
	an.clear();
	for(auto& e : bsdata<quest>()) {
		if(e.stage != quest::Active)
			continue;
		an.add(&e, e.getname());
	}
	an.sort();
	last_quest = (quest*)choose_large_menu(get_title("PartyAdventure", "Options"), getnm("Cancel"));
	an = push_answers;
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
	update_party_position();
}

static void turn_left() {
	party.d = to(party.d, Left);
	update_party_position();
}

static void explore_area() {
	if(!loc)
		return;
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

static bool use_tool_item(abilityn skill, int bonus, int chance_good_shape = 35) {
	if(player->roll(skill, bonus))
		return true;
	consolen(getnm(ids(bsdata<abilityi>::elements[skill].id, "Fail")));
	last_item->usecharge("ToolBroken", chance_good_shape);
	return false;
}

static void use_tool_success(celln n, int add_experience = 100) {
	if(add_experience)
		player->addexp(add_experience);
	consolen(getnm(ids(bsdata<celli>::elements[n].id, "Disable")));
}

static int get_level_difficult() {
	auto n = loc->level;
	if(n <= 1)
		return 0;
	return -(n - 1) * 5;
}

static void use_theif_tools(int bonus) {
	auto p = loc->get(*player, player->d);
	if(p) {
		switch(p->type) {
		case CellKeyHole:
			if(active_overlay(p, true))
				return;
			if(use_tool_item(OpenLocks, get_level_difficult())) {
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
			if(use_tool_item(RemoveTraps, get_level_difficult())) {
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
	player->speak("TheifTool", "NoTargets");
}

static void use_grappling_hook(int bonus) {
	switch(loc->get(to(*player, player->d))) {
	case CellPit:
		if(use_tool_item(ClimbWalls, get_level_difficult(), 15)) {
			loc->set(to(party, party.d), CellPassable);
			use_tool_success(CellPit, 20);
		}
		pass_round();
		return;
	default:
		break;
	}
	if(locup) {
		switch(locup->get(*player)) {
		case CellPit:
			//if(use_tool_item(ClimbWalls, get_level_difficult(), 15)) {
			//	loc->set(to(party, party.d), CellPassable);
			//	use_tool_success(CellPit, 20);
			//}
			pass_round();
			return;
		default:
			break;
		}
	}
	player->speak("GrapplingHook", "NoTargets");
}

static bool manipulate_overlay() {
	auto p = loc->get(*player, player->d);
	if(!p)
		return false;
	auto v = to(*player, player->d);
	auto pi = (item*)current_focus;
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
		if(change_overlay(*player, player->d)) {
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
	return true;
}

static bool manipulate_cell() {
	auto v = to(*player, player->d);
	auto t = loc->get(v);
	auto id = getid<celli>(t);
	switch(t) {
	case CellPortal:
		player->speak(id, "About");
		if(player->is(UseMage))
			apply_script(id, "Use", 0);
		break;
	case CellBarel:
	case CellWeb:
	case CellCocon:
		player->speak(id, "About");
		apply_script(id, "Use", 0);
		break;
	case CellGrave:
		broke_cell(v);
		break;
	default:
		return false;
	}
	return true;
}

static void manipulate() {
	if(!player)
		return;
	if(!player->isactable())
		return;
	if(manipulate_overlay() || manipulate_cell())
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
		{'U', use_item},
		{'E', cast_spell},
		{'R', change_quick_item},
		{KeyEscape, choose_dungeon_menu},
#ifdef _DEBUG
		{'T', test_ground},
#endif
		{}};
	adventure_input(source);
	set_player_by_focus();
}

static void play_dungeon() {
	current_focus = save_focus;
	set_player_by_focus();
	show_scene(paint_adventure, play_dungeon_input, save_focus);
}

static quest* find_quest(dungeoni* p) {
	for(auto& e : bsdata<quest>()) {
		if(!e)
			continue;
		for(auto& d : e.dungeon) {
			if(&d == p)
				return &e;
		}
	}
	return 0;
}

static void enter_active_dungeon() {
	locup = 0;
	if(loc->level > 1)
		locup = loc - 1;
	last_quest = find_quest(loc);
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

static dungeoni* choose_teleport_target() {
	pushanswer push;
	for(auto& e : bsdata<quest>()) {
		if(!e || e.stage != quest::Active)
			continue;
		for(auto& d : e.dungeon) {
			if(!d.state.portal)
				continue;
			if(!d.is(d.state.portal, CellExplored))
				continue;
			an.add(&e, e.getname());
			break;
		}
	}
	last_result = choose_large_menu(getnm("TeleportChoosePlace"), getnm("Cancel"));
	if(!last_result)
		return 0;
	an.clear();
	for(auto& e : ((quest*)last_result)->dungeon) {
		if(!e.state.portal)
			continue;
		if(!e.is(e.state.portal, CellExplored))
			continue;
		an.add(&e, getnm("TeleportAskLevel"), e.level);
	}
	return (dungeoni*)choose_large_menu(getnm("TeleportChooseLevel"), getnm("Cancel"));
}

static void portal_teleportation(int bonus) {
	auto p = choose_teleport_target();
	if(!p) {
		script_stop();
		return;
	}
	auto d = p->getnear(p->state.portal, CellPassable);
	if(d == Center)
		return;
	if(bonus)
		all_party(craft_mission_equipment, true);
	loc = p;
	set_party_position(to(loc->state.portal, d), d);
	enter_active_dungeon();
	consolen(getnm("PartyPortalTeleportation"));
}

static void pit_fall_down() {
	if(!player->roll(ClimbWalls))
		player->damage(Bludgeon, xrand(3, 18));
}

static bool party_move_interact(pointc v) {
	auto t = loc->get(v);
	switch(t) {
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
		set_party_position(v);
		loc = find_dungeon(loc->level + 1);
		animation_update();
		all_party(pit_fall_down, true);
		consolen(getnm("PartyFallPit"));
		enter_active_dungeon();
		animation_update();
		pass_round();
		break;
	case CellOverlay1:
	case CellOverlay2:
	case CellOverlay3:
		apply_script(getid<celli>(t), "Use", 0);
		break;
	default:
		return false;
	}
	return true;
}

static void talk_monsters(const char* format) {
	auto pm = opponent->getmonster();
	if(!pm)
		return;
	if(!last_quest)
		return;
	auto rm = bsdata<reactioni>::elements[last_reaction].id;
	pushanswer push;
	auto pe = bsdata<listi>::find(ids(last_quest->id, rm));
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
	auto rm = bsdata<reactioni>::elements[last_reaction].id;
	if(!last_quest)
		return false;
	auto pn = speech_get_na(pm->id, last_quest->id);
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
		opponent = get_leader(creatures);
		if(!opponent)
			break;
		check_reaction(creatures, bonus);
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
	all_party(craft_mission_equipment, true);
	enter_dungeon(0);
}

static void party_leave(int bonus) {
	if(!last_dungeon)
		return;
	loc = last_dungeon;
	set_party_position(last_exit, to(last_exit.d, Down));
	enter_active_dungeon();
}

void continue_game() {
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

static void instant_kill(int bonus) {
	player->kill();
}

static void curse_item(int bonus) {
	last_item->curse(bonus);
}

static void create_power(int bonus) {
	if(bonus > 0)
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

static void learn_mage_spells(int bonus) {
	auto ps = get_spells_known(player);
	if(!ps)
		return;
	update_player();
	if(!can_cast_spell(1, bonus))
		return;
	pushanswer push;
	add_spells(1, bonus, 0);
	an.elements.shuffle();
	an.elements.top(5);
	for(auto& e : an.elements) {
		if(player->roll(LearnSpell))
			ps->set(getbsi((spelli*)e.value));
	}
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

static void pay_gold_confirm(int bonus) {
	if(!confirm_payment("Pay", get_bonus(bonus)))
		script_stop();
}

static void apply_racial_enemy(int bonus) {
	if(!last_race)
		return;
	if(bonus >= 0)
		player->hate.set(last_race);
	else
		player->hate.remove(last_race);
}

static void player_speak(const char* action) {
	player->speakn(last_id, action);
}

static void read_story(int bonus) {
	auto postfix = "Success";
	auto format = npc_speech(player, last_id, postfix);
	if(!format)
		return;
	consolen(getnm("PlayerRead"));
	console(" ");
	console(format);
}

static void say_speech(int bonus) {
	switch(bonus) {
	case -1: player_speak("Fail"); break;
	default: player_speak("Success"); break;
	}
}

static void make_roll(int bonus) {
	if(player->roll(last_ability, get_bonus(bonus) * 5)) {
		dialog_message("Success");
		player_speak("SuccessSpeech");
	} else {
		script_stop();
		dialog_message("Fail");
		player_speak("FailSpeech");
		apply_script(last_id, "Fail", 0);
	}
}

static void make_blessing_roll(int bonus) {
	if(roll_ability(getparty(Blessing) + (get_bonus(bonus) * 5)))
		dialog_message("Success");
	else {
		script_stop();
		dialog_message("Fail");
		apply_script(last_id, "Fail", 0);
	}
}

static void make_roll_average(int bonus) {
	if(party_roll(last_ability, get_bonus(bonus))) {
		dialog_message("Success");
		player_speak("SuccessSpeech");
	} else {
		script_stop();
		dialog_message("Fail");
		player_speak("FailSpeech");
		apply_script(last_id, "Fail", 0);
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
	party_addexp(bonus * 400);
}

static void add_exp_group(int bonus) {
	party_addexp(bonus * 100);
}

static void add_exp_personal(int bonus) {
	player->addexp(bonus * 100);
}

static void add_exp_evil(int bonus) {
	party_addexp(LawfulEvil, bonus * 40);
	party_addexp(NeutralEvil, bonus * 40);
	party_addexp(ChaoticEvil, bonus * 40);
}

static void add_exp_good(int bonus) {
	party_addexp(LawfulGood, bonus * 50);
	party_addexp(NeutralGood, bonus * 40);
	party_addexp(ChaoticGood, bonus * 40);
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
		apply_script(last_id, "Fail", 0);
	}
}

static void save_vs_poison_negate(int bonus) {
	if(player->roll(SaveVsPoison, bonus * 5)) {
		last_number = 0;
		script_stop();
		apply_script(last_id, "Fail", 0);
	}
}

static void save_half(int bonus) {
	if(player->roll(SaveVsMagic, bonus * 5))
		last_number = last_number / 2;
}

static void apply_chance(int bonus) {
	if(d100() >= get_bonus(bonus))
		script_stop();
}

static void set_caster(int bonus) {
	player = caster;
}

static void set_character(int bonus) {
	player = characters[bonus];
}

static void push_player(int bonus) {
	auto push = player;
	script_run();
	player = push;
}

static void push_quest(int bonus) {
	auto push = last_quest;
	script_run();
	last_quest = push;
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

static void monster_leader(int bonus) {
	creaturei* creatures[6]; loc->getmonsters(creatures, to(party, party.d));
	player = get_leader(creatures);
	if(!player)
		script_stop();
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

static void player_name(stringbuilder& sb) {
	sb.add(player->getname());
}

static void diety_name(stringbuilder& sb) {
	sb.add("Helm");
}

static void player_gender(stringbuilder& sb) {
	sb.add(getnm(getid<genderi>(player->gender)));
}

static void player_class(stringbuilder& sb) {
	sb.add(getnm(getid<classi>(player->character_class)));
}

static void player_race(stringbuilder& sb) {
	sb.add(getnm(getid<racei>(player->race)));
}

static void opponent_name(stringbuilder& sb) {
	sb.add(opponent->getname());
}

static int total_count(const variants& effect, variant type) {
	auto result = 0;
	for(auto v : effect) {
		if(v.type == type.type && v.value == type.value)
			result += v.counter;
	}
	return result;
}

static void quest_reward_gold(stringbuilder& sb) {
	auto result = total_count(last_quest->reward, "AddReward") * 100;
	sb.add("%1i", result);
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

static bool if_zero() {
	return last_number == 0;
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

static bool if_item_can_learn_spell() {
	auto power = last_item->getpower();
	if(power.iskind<spelli>())
		return can_learn_spell(1, bsdata<spelli>::elements[power.value].levels[1]);
	return false;
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
	auto pm = opponent->getmonster();
	if(!pm)
		return false;
	auto monster_id = getbsi(pm);
	auto current_stage = last_quest->gethistory(monster_id);
	auto pn = get_stage_text(last_quest, monster_id, current_stage + 1, 0);
	if(!pn)
		return false;
	if(run) {
		dialog(0, pn);
		auto ph = last_quest->addhistory(monster_id);
		if(ph)
			ph->value = current_stage + 1;
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
	{"Class", player_class},
	{"Diety", diety_name},
	{"DungeonBoss", dungeon_boss},
	{"DungeonKey", dungeon_key},
	{"DungeonOrigin", dungeon_origin},
	{"DungeonSpecial", dungeon_special},
	{"Gender", player_gender},
	{"Habbitant1", dungeon_habbitant1},
	{"Habbitant2", dungeon_habbitant2},
	{"ItemName", item_name},
	{"Name", player_name},
	{"OpponentName", opponent_name},
	{"Number", effect_number},
	{"QuestRewardGold", quest_reward_gold},
	{"Race", player_race},
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
	{"IfItemCanLearnSpell", if_item_can_learn_spell},
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
	{"IfZero", if_zero},
};
BSDATAF(conditioni)
BSDATA(script) = {
	{"AllLanguages", all_languages},
	{"Attack", attack_modify},
	{"ActionItems", action_items},
	{"ActionPlayerItems", action_player_items},
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
	{"ApplyCarousing", apply_carousing},
	{"ApplyEnchantSpell", apply_enchant_spell},
	{"ApplyRacialEnemy", apply_racial_enemy},
	{"BestPlayer", best_player},
	{"BuyMenu", buy_menu},
	{"Caster", set_caster},
	{"ConfirmAction", confirm_action},
	{"Chance", apply_chance},
	{"Character", set_character},
	{"ChooseAlignment", choose_alignment},
	{"ChooseClass", choose_class},
	{"ChooseGender", choose_gender},
	{"ChooseItems", choose_items},
	{"ChooseMenu", choose_menu},
	{"ChooseRace", choose_race},
	{"ChooseShopItem", choose_shop_item},
	{"ChooseSpells", choose_spells},
	{"ClearArea", clear_area},
	{"ClearGame", clear_game},
	{"CreateCharacter", create_character},
	{"CreatePower", create_power},
	{"CurseItem", curse_item},
	{"Damage", damage_modify},
	{"DamageItem", damage_item},
	{"DestroyItem", destroy_item},
	{"ExitGame", exit_game},
	{"EnterDungeon", enter_dungeon},
	{"EnterLocation", enter_location},
	{"EnterSanctuary", enter_sanctuary},
	{"FilterArea", filter_area},
	{"FilterAreaExplored", filter_area_explored},
	{"ForEachItem", for_each_item},
	{"ForEachOpponents", for_each_opponents},
	{"ForEachParty", for_each_party},
	{"GenerateParty", generate_party},
	{"GrapplingHookUse", use_grappling_hook},
	{"Heal", player_heal},
	{"HealEffect", player_heal_effect},
	{"IdentifyItem", identify_item},
	{"InstantKill", instant_kill},
	{"ItemPowerSpell", item_power_spell},
	{"Leader", monster_leader},
	{"LearnClericSpells", learn_cleric_spells},
	{"LearnLastSpell", learn_last_spell},
	{"LearnMageSpells", learn_mage_spells},
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
	{"PartyLeave", party_leave},
	{"PartyUnlock", party_unlock},
	{"PayGold", pay_gold},
	{"PayGoldConfirm", pay_gold_confirm},
	{"PassDays", pass_hours},
	{"PassHours", pass_hours},
	{"PassRound", pass_round},
	{"PortalTeleportation", portal_teleportation},
	{"Protection", protection_modify},
	{"PushItem", push_item},
	{"PushModifier", push_modifier},
	{"PushPlayer", push_player},
	{"PushQuest", push_quest},
	{"RandomArea", random_area},
	{"ReactionCheck", reaction_check},
	{"ReadStory", read_story},
	{"RestoreSpells", restore_spells},
	{"ReturnToStreet", return_to_street},
	{"Roll", make_roll},
	{"RollAverage", make_roll_average},
	{"RollBlessing", make_blessing_roll},
	{"Satisfy", satisfy},
	{"SaySpeech", say_speech},
	{"SaveGame", save_game},
	{"Saves", saves_modify},
	{"SaveHalf", save_half},
	{"SaveNegate", save_negate},
	{"SaveVsPoisonNegate", save_vs_poison_negate},
	{"SelectArea", select_area},
	{"SetLevel", set_level},
	{"SetVariable", set_variable},
	{"ShowArea", show_area},
	{"SleepParty", sleep_party},
	{"StrenghtAdd", strenght_add},
	{"Switch", apply_switch},
	{"TalkAbout", talk_about},
	{"TheifToolsUse", use_theif_tools},
	{"TurningMonsters", turning_monsters},
	{"Wizardy", wizardy_effect},
};
BSDATAF(script)