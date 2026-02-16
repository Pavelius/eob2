#include "bsdata.h"
#include "bsreq.h"
#include "draw.h"
#include "keybind.h"
#include "stringbuilder.h"

BSDATAC(keybindi, 64)
BSMETA(keybindi) = {
	BSREQ(type),
	BSREQ(id_key),
	BSREQ(command),
	{}};

namespace {
struct keymapi {
	const char*	key;
	unsigned	value;
};
}

static keymapi keys[] = {
	{"Back", KeyBackspace},
	{"Del", KeyDelete},
	{"Enter", KeyEnter},
	{"Esc", KeyEscape},
	{"F1", F1},
	{"F2", F2},
	{"F3", F3},
	{"F4", F4},
	{"F5", F5},
	{"F6", F6},
	{"F7", F7},
	{"F8", F8},
	{"F9", F9},
	{"F10", F10},
	{"F11", F11},
	{"F12", F12},
	{"Space", KeySpace},
	{"Tab", KeyTab},
};

static keymapi* find_key(const char* id) {
	for(auto& e : keys) {
		if(equal(e.key, id))
			return &e;
	}
	return 0;
}

static unsigned parse_keybing(const char* key_string) {
	if(!key_string || !key_string[0])
		return 0;
	unsigned result = 0;
	for(auto& e : keys) {
		auto p = find_key(key_string);
		if(p) {
			result |= e.value;
			key_string += zlen(e.key);
		} else {
			result |= (unsigned char)key_string[0];
			key_string += 1;
		}
		if(key_string[0] == '+') {
			key_string++;
			continue;
		}
		break;
	}
	return result;
}

void initialize_keybind() {
	for(auto& e : bsdata<keybindi>())
		e.key = parse_keybing(e.id_key);
}