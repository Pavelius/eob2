#include "creature.h"
#include "textscript.h"
#include "speech.h"

bool parse_abilities(stringbuilder& sb, const char* id);
bool parse_speech(stringbuilder& sb, const char* id);
bool parse_wall_messages(stringbuilder& sb, const char* id);

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
