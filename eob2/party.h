#pragma once

#include "adat.h"
#include "flagable.h"
#include "nameable.h"
#include "posable.h"

typedef flagable<8, unsigned> questa;

struct creaturei;

enum abilityn : unsigned char;

enum partystatn : unsigned char {
	GoldPiece, Reputation, Blessing,
	StartYear, StartDeadLine, StopDeadLine, Minutes,
	EffectCount,
};
struct partystati : nameable {
};
struct partyi : posable {
	short unsigned	location;
	questa			active, done, prepared;
	int				abilities[EffectCount + 1];
};
extern creaturei* characters[6];
extern partyi party;
extern partystatn last_variable;

void add_party(partystatn id, int value);
void continue_game();
void delete_game(const char* id);
bool is_dead_line();
void join_party(int bonus);
void make_melee_attacks();
void monster_interaction();
void move_party(pointc v);
void party_addexp(int value);
void party_addexp_per_killed(int victim_hit_die);
void pass_hours(int count);
void pass_round();
bool read_game(const char* id);
void save_game(const char* id);
void set_party_position(pointc v);
void set_party_position(pointc v, directions d);
void surprise_roll(creaturei** creatures, int bonus);
void turnto(pointc v, directions d, bool test_surprise = false, int sneaky_bonus = 0);

int getparty(partystatn id);
int get_party_index(const creaturei* target);
int party_median(creaturei** creatures, abilityn v);

char* get_spells_prepared(const creaturei* target);