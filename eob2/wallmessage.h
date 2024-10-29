#pragma once

enum messages : unsigned char {
	MessageMagicWeapons, MessageMagicRings, MessageSecrets, MessageTraps, MessageLocked,
	MessageAtifacts, MessageSpecialItem, MessageBoss,
	MessageHabbits
};
struct wallmessagei {
   const char* id;
};
