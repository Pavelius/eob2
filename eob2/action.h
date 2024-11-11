#pragma once

#include "flagable.h"
#include "nameable.h"
#include "picture.h"
#include "variant.h"

struct creaturei;

struct actioni : nameable {
	picturei	avatar;
	flag32		restrict_classes, classes, races, alignment;
	variants	filter, effect;
	bool isallow(const creaturei* player) const;
};
extern actioni* last_action;
