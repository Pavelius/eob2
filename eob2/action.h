#pragma once

#include "nameable.h"
#include "flagable.h"
#include "variant.h"

struct actioni : nameable {
	flagable<4> classes, races, alignment;
	variants effect;
};
