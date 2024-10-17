#pragma once

enum directions : unsigned char {
	Center, Left, Up, Right, Down
};
struct directioni {
	const char*		name;
};
directions to(directions v, directions d);