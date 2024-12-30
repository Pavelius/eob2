#include "bsdata.h"
#include "modifier.h"
#include "script.h"

BSDATA(modifieri) = {
	{"Standart"},
	{"Wearing"},
	{"Permanent"},
};
assert_enum(modifieri, Permanent)