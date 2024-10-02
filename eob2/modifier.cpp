#include "bsdata.h"
#include "modifier.h"
#include "script.h"

BSDATA(modifieri) = {
	{"Standart"},
	{"Permanent"},
};
assert_enum(modifieri, Permanent)