#pragma once

enum directions : unsigned char {
	Center, Left, Up, Right, Down
};
struct directioni {
	const char*	id;
};
directions to(directions v, directions d);