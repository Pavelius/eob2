#pragma once

#include "nameable.h"
#include "flagable.h"

struct actioni : nameable {
	flagable<4> classes, races, alignment;
};
