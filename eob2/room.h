#pragma once

#include "corridor.h"
#include "shape.h"

typedef void (*fnroom)(pointc v, directions d, const shapei* p);

struct roomi : nameable {
	shapei*		shape;
};
