#pragma once

#include "adat.h"
#include "flagable.h"
#include "nameable.h"
#include "posable.h"

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
struct partystata {
	int				abilities[EffectCount + 1];
};
struct partystati : nameable {
	int				maximum;
};
struct partyi : posable, partystata {
	int				unlock[Blessing + 1];
	short unsigned	location_id;
	void			clear();
	locationi*		getlocation() const;
};
extern creaturei* characters[6];
extern creaturei** player_position;
extern partyi party;
extern partystatn last_variable;
extern int enemy_distance;
extern slice<unsigned char> character_avatars;

unsigned get_stamp(unsigned duration);

void all_creatures(fnevent proc);
void all_party(fnevent proc, bool skip_disabled);
void add_party(partystatn id, int value);
bool apply_message(const char* id, const char* action);
void broke_cell(pointc v);
void check_reaction(creaturei** creatures, int bonus);
void clear_game(int bonus);
void continue_game();
void delete_game(const char* id);
bool is_dead_line();
bool is_party_full();
bool ismatch(char* abilitites);
void join_party(int bonus);
void join_party();
void main_menu();
void make_attacks(bool melee_combat);
bool make_object_attack(pointc v);
void move_party(pointc v);
void party_addexp(int value);
void party_addexp_per_killed(int victim_hit_die);
void party_addexp(alignmentn v, int value);
bool party_is(alignmentn v);
bool party_is(featn v);
bool party_is(creaturei* player);
bool party_roll(abilityn v, int bonus);
void party_say(const char* id, const char* action);
void pass_hours(int count);
void pass_round();
void reaction_check(int bonus);
bool read_game(const char* id);
void replace_character();
void save_game(const char* id);
void set_party_position(pointc v);
void set_party_position(pointc v, directions d);
void set_reaction(creaturei** creatures, reactions v);
void surprise_roll(creaturei** creatures, int bonus);
void turnto(pointc v, directions d, bool test_surprise = false, int sneaky_bonus = 0);
void update_default_spells();
void update_party_position();
void use_item();

int getparty(partystatn id);
int get_party_index(const creaturei* target);
int party_goal(goaln v);
int party_best(creaturei** creatures, abilityn v, bool set_player);
int party_median(creaturei** creatures, abilityn v);

char* get_spells_prepared(const creaturei* target);

item* party_get_item(const itemi* pi);

reactions get_reaction(creaturei** creatures);

creaturei* get_leader(creaturei** creatures);