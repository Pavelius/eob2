#pragma once

enum messages : unsigned char {
	MessageMagicWeapons, MessageMagicRings, MessageSecrets, MessageTraps, MessageLocked,
	MessageAtifacts, MessageCursedItems, MessageSpecialItem, MessageBoss,
	MessageHabbits
};
struct wallmessagei {
   const char* id;
};
