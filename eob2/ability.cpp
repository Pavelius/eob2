#include "ability.h"
#include "bsdata.h"
#include "feat.h"

BSDATA(abilityi) = {
	{"Strenght", 100},
	{"Dexterity", 100},
	{"Constitution", 100},
	{"Intellegence", 100},
	{"Wisdow", 100},
	{"Charisma", 100},
	{"SaveVsParalization", 10},
	{"SaveVsPoison", 10},
	{"SaveVsTraps", 10},
	{"SaveVsMagic", 10},
	{"ClimbWalls", 50, TheifSkills},
	{"HearNoise", 50, TheifSkills},
	{"MoveSilently", 50, StealthSkills},
	{"OpenLocks", 50, TheifSkills},
	{"PickPockets", 50, TheifSkills},
	{"RemoveTraps", 50, TheifSkills},
	{"ReadLanguages", 20, TheifSkills},
	{"LearnSpell", 40},
	{"ResistMagic", 20},
	{"Sneaky", 1},
	{"Alertness", 1},
	{"CriticalDeflect", 10},
	{"DetectSecrets", 10},
	{"AC", 1},
	{"AttackMelee", 1},
	{"AttackRange", 1},
	{"DamageMelee", 1},
	{"DamageRange", 1},
	{"Speed"},
	{"TurnUndeadBonus", 1},
	{"Backstab", 1},
	{"AdditionalAttacks", 1},
	{"Spell1", 1},
	{"Spell2", 1},
	{"Spell3", 1},
	{"Spell4", 1},
	{"Spell5", 1},
	{"Spell6", 1},
	{"Spell7", 1},
	{"Spell8", 1},
	{"Spell9", 1},
	{"Spells", 1},
	{"BonusExperience"},
	{"ReactionBonus"},
	{"ExeptionalStrenght"},
	{"AcidD1Level"},
	{"AcidD2Level"},
	{"PoisonLevel"},
	{"DiseaseLevel"},
	{"DuplicateIllusion"},
	{"DrainStrenght"},
	{"DrainConstitution"},
	{"DrainLevel"},
	{"Hits"},
};
assert_enum(abilityi, Hits)

abilityn last_ability;