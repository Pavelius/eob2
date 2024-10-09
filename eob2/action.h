#pragma once

#include "flagable.h"
#include "nameable.h"
#include "picture.h"
#include "variant.h"

struct creaturei;

struct actioni : nameable {
	picturei	avatar;
	flag32		classes, races, alignment;
	variants	effect;
	bool isallow(const creaturei* player) const;
};
extern actioni* last_action;
