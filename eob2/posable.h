#pragma once

enum directions : unsigned char;

struct pointc {
	char		x = -1, y = -1;
};
struct posable : pointc {
	char		side = 0;
	directions	direction = (directions)0;
};
