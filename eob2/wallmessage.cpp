#include "bsdata.h"
#include "wallmessage.h"

BSDATA(wallmessagei) = {
	{"MessageMagicWeapons"},
	{"MessageMagicRings"},
	{"MessageSecrets"},
	{"MessageTraps"},
	{"MessageLocked"},
	{"MessageAtifacts"},
	{"MessageCursedItems"},
	{"MessageSpecialItem"},
	{"MessageBoss"},
	{"MessageHabbits"},
};
assert_enum(wallmessagei, MessageHabbits)
