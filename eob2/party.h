#pragma once

#include "flagable.h"
#include "nameable.h"
#include "posable.h"

typedef flagable<8, unsigned> questa;

struct creaturei;

enum partystatn : unsigned char {
	GoldPiece, Reputation, Blessing,
	StartYear, StartDeadLine, StopDeadLine, Minutes
};
struct partystati : nameable {
};
struct partyi : posable {
	short unsigned	location;
	questa			active, done, prepared;
	int				abilities[Minutes + 1];
};
extern creaturei* characters[6];
extern partyi party;

void add_party(partystatn id, int value);
void continue_game();
void delete_game(const char* id);
bool is_dead_line();
void join_party(int bonus);
void move_party(pointc v);
void party_addexp(int value);
void pass_round();
bool read_game(const char* id);
void save_game(const char* id);
void set_party_position(pointc v);
void set_party_position(pointc v, directions d);
void skip_hours(int value);

int getparty(partystatn id);
int get_party_index(const creaturei* target);
