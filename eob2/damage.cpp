#include "bsdata.h"
#include "damage.h"

BSDATA(damagei) = {
	{"Bludgeon"},
	{"Slashing"},
	{"Piercing"},
	{"Fire"},
	{"Cold"},
	{"Acid"},
};
assert_enum(damagei, Acid)
