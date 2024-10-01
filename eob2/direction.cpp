#include "bsdata.h"
#include "direction.h"

directioni bsdata<directioni>::elements[] = {
	{"Center"},
	{"Left"},
	{"Up"},
	{"Right"},
	{"Down"},
};
assert_enum(directioni, Down)