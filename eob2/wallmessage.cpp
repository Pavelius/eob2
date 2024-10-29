#include "bsdata.h"
#include "wallmessage.h"

BSDATA(wallmessagei) = {
	{"MessageMagicWeapons"},
	{"MessageMagicRings"},
	{"MessageSecrets"},
	{"MessageTraps"},
	{"MessageLocked"},
	{"MessageAtifacts"},
	{"MessageSpecialItem"},
	{"MessageBoss"},
	{"MessageHabbits"},
};
assert_enum(wallmessagei, MessageHabbits)
