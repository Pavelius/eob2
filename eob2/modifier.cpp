#include "bsdata.h"
#include "modifier.h"
#include "script.h"

BSDATA(modifieri) = {
	{"Standart"},
	{"Wearing"},
	{"Grounding"},
	{"Permanent"},
};
assert_enum(modifieri, Permanent)