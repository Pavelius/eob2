#pragma once

#include "flagable.h"
#include "nameable.h"
#include "posable.h"
#include "unit.h"

typedef flagable<8, unsigned> questa;

enum partystatn : unsigned char {
	GoldPiece, Reputation, Blessing,
	StartYear, StartDeadLine, StopDeadLine, Minutes
};
struct partystati : nameable {
};
struct partyi : uniti, posable {
	short unsigned	location;
	questa			active, done;
	int				abilities[Minutes + 1];
};
extern partyi party;

void add_party(partystatn id, int value);
void continue_game();
bool is_dead_line();
bool read_game(const char* id);
void save_game(const char* id);
void skip_hours(int value);

int getparty(partystatn id);
