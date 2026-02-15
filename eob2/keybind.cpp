#include "bsdata.h"
#include "keybind.h"
#include "log.h"

BSDATAC(keybindi, 64)

static unsigned parse_keybing(const char* key_string) {
	return 0;
}

void initialize_keybind() {
	for(auto& e : bsdata<keybindi>()) {
		if(!e.shortcut && !e.key)
			e.key = parse_keybing(e.shortcut);
	}
}