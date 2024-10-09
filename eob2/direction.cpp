#include "bsdata.h"
#include "direction.h"

BSDATA(directioni) = {
	{"Center"},
	{"Left"},
	{"Up"},
	{"Right"},
	{"Down"},
};
assert_enum(directioni, Down)
