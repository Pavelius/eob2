#include "bsdata.h"
#include "reaction.h"

reactions last_reaction;

BSDATA(reactioni) = {
	{"Indifferent"},
	{"Friendly"},
	{"Careful"},
	{"Hostile"},
};
assert_enum(reactioni, Hostile)