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

directions to(directions v, directions d) {
	static const directions rotate_direction[4][4] = {
		{Down, Left, Up, Right},
		{Left, Up, Right, Down},
		{Up, Right, Down, Left},
		{Right, Down, Left, Up},
	};
	return rotate_direction[v - Left][d - Left];
}
