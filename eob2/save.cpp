#include "bsdata.h"
#include "save.h"

BSDATA(savei) = {
	{"NoSave"},
	{"SaveHalf"},
	{"SaveAttack"},
	{"SaveNegate"},
};
assert_enum(savei, SaveNegate)