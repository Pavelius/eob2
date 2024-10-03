#include "bsdata.h"
#include "damage.h"

BSDATA(damagei) = {
	{"Bludgeon"},
	{"Slashing"},
	{"Pierce"},
	{"Fire"},
	{"Cold"},
	{"Acid"},
};
assert_enum(damagei, Acid)
