#pragma once

#include "nameable.h"
#include "unit.h"

enum partystatn : unsigned char {
	GoldPiece, Reputation, Blessing,
	StartYear, StartDeadLine, StopDeadLine, Minutes
};
struct partystati : nameable {
};
struct partyi : uniti {
	short unsigned	location;
	int				abilities[Minutes + 1];
};
extern partyi party;

void add_party(partystatn id, int value);
void join_party();
bool is_dead_line();
void skip_hours(int value);

int getparty(partystatn id);
