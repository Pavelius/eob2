#include "ability.h"
#include "bsdata.h"

BSDATA(abilityi) = {
	{"Strenght"},
	{"Dexterity"},
	{"Constitution"},
	{"Intellegence"},
	{"Wisdow"},
	{"Charisma"},
	{"SaveVsParalization"},
	{"SaveVsPoison"},
	{"SaveVsTraps"},
	{"SaveVsMagic"},
	{"ClimbWalls"},
	{"HearNoise"},
	{"MoveSilently"},
	{"OpenLocks"},
	{"RemoveTraps"},
	{"ReadLanguages"},
	{"LearnSpell"},
	{"ResistMagic"},
	{"CriticalDeflect"},
	{"DetectSecrets"},
	{"AC"},
	{"AttackMelee"},
	{"AttackRange"},
	{"DamageMelee"},
	{"DamageRange"},
	{"Speed"},
	{"BonusExperience"},
	{"ReactionBonus"},
	{"ExeptionalStrenght"},
	{"Hits"},
};
assert_enum(abilityi, Hits)

void add_value(char& result, int i, int minimum, int maximum) {
	i += result;
	if(i < minimum)
		i = minimum;
	else if(i > maximum)
		i = maximum;
	result = i;
}