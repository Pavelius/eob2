#include "creature.h"
#include "dungeon.h"
#include "gender.h"
#include "textscript.h"
#include "speech.h"

bool parse_abilities(stringbuilder& sb, const char* id);
bool parse_speech(stringbuilder& sb, const char* id);

static bool parse_gender(stringbuilder& sb, const char* identifier, gendern gender) {
	struct gender_change_string {
		const char*	female;
		const char*	male;
		const char*	multiply;
	};
	static gender_change_string source[] = {
		{"вона", "він", "вони"},
		{"лась", "вся", "лись"},
		{"леді", "лорд", "лорди"},
		{"така", "такий", "такі"},
		{"неї", "нього", "них"},
		{"її", "його", "їх"},
		{"їй", "йому", "їм"},
		{"ла", "в", "ли"},
		{"а", "", "и"},
	};
	for(auto& e : source) {
		if(!equal(e.female, identifier))
			continue;
		if(gender == NoGender)
			sb.add(e.multiply);
		else if(gender == Female)
			sb.add(e.female);
		else
			sb.add(e.male);
		return true;
	}
	return false;
}

static bool parse_wall_messages(stringbuilder& sb, const char* id) {
	if(!loc)
		return false;
	auto pn = bsdata<wallmessagei>::find(id);
	if(!pn)
		return false;
	auto index = pn - bsdata<wallmessagei>::elements;
	sb.add("%1i", loc->state.wallmessages[index]);
	return true;
}

static bool parse_script(stringbuilder& sb, const char* id) {
	for(auto& e : bsdata<textscript>()) {
		if(!equal(e.id, id))
			continue;
		e.proc(sb);
		return true;
	}
	return false;
}

static void custom_string(stringbuilder& sb, const char* id) {
	if(parse_script(sb, id))
		return;
	if(parse_gender(sb, id, player->gender))
		return;
	if(parse_speech(sb, id))
		return;
	if(parse_abilities(sb, id))
		return;
	if(parse_wall_messages(sb, id))
		return;
	sb.add(getnm(id));
}

void initialize_strings() {
	stringbuilder::custom = custom_string;
}
