#pragma once

#include "adat.h"
#include "flagable.h"
#include "nameable.h"
#include "posable.h"

typedef flagable<8, unsigned> questa;

class item;

struct creaturei;
struct itemi;
struct locationi;
struct quest;

enum abilityn : unsigned char;
enum alignmentn : unsigned char;
enum featn : unsigned char;
enum goaln : unsigned char;
enum reactions : unsigned char;

enum partystatn : unsigned char {
	GoldPiece, Reputation, Blessing,
	StartYear, StartDeadLine, StopDeadLine, Minutes,
	EffectCount,
};
struct partystati : nameable {
};
struct partyi : posable {
	short unsigned	location_id, quest_id;
	questa			active, done, prepared;
	unsigned char	stages[256];
	int				abilities[EffectCount + 1];
	void			clear();
	locationi*		getlocation() const;
	int				getstage(unsigned char v) const { return stages[quest_id]; }
	int				getstage(const quest* v) const;
	quest*			getquest() const;
};
extern creaturei* characters[6];
extern creaturei* monsters[6];
extern partyi party;
extern partystatn last_variable;
extern int enemy_distance;

unsigned get_stamp(unsigned duration);

void all_creatures(fnevent proc);
void all_party(fnevent proc, bool skip_disabled);
void add_party(partystatn id, int value);
void check_reaction(creaturei** creatures);
void continue_game();
void delete_game(const char* id);
bool is_dead_line();
void join_party(int bonus);
void main_menu();
void make_attacks(bool melee_combat);
void monster_interaction();
void move_party(pointc v);
void party_addexp(int value);
void party_addexp_per_killed(int victim_hit_die);
void party_addexp(alignmentn v, int value);
bool party_is(alignmentn v);
bool party_is(featn v);
void pass_hours(int count);
void pass_round();
bool read_game(const char* id);
void save_game(const char* id);
void set_party_position(pointc v);
void set_party_position(pointc v, directions d);
void set_reaction(creaturei** creatures, reactions v);
void surprise_roll(creaturei** creatures, int bonus);
void turnto(pointc v, directions d, bool test_surprise = false, int sneaky_bonus = 0);

int getparty(partystatn id);
int get_party_index(const creaturei* target);
int party_goal(unsigned short quest_id, goaln v);
int party_median(creaturei** creatures, abilityn v);

char* get_spells_prepared(const creaturei* target);

item* party_get_item(const itemi* pi);

reactions get_reaction(creaturei** creatures);