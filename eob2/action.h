#pragma once

#include "flagable.h"
#include "nameable.h"
#include "party.h"
#include "picture.h"
#include "variant.h"

struct creaturei;

enum classn : unsigned char;

struct actioni : nameable {
	picturei	avatar;
	flag32		restrict_classes, classes, races, alignment;
	variants	filter, effect;
	char		required[Blessing + 1];
	bool isallow(const creaturei* player) const;
};
extern actioni* last_action;

bool have_class(const flag32& classes, classn type);
