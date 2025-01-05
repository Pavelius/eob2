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
	variants	filter, filter_item, effect;
	char		required[Blessing + 1];
	bool		isallow(const creaturei* player) const;
};
extern actioni* last_action;

bool allow_item(const variants& filter);
bool allow_item(creaturei* player, const variants& filter);
bool have_class(const flag32& classes, classn type);
bool party_have_alignment(flag32 source);
bool party_have_class(flag32 source);
bool party_have_race(flag32 source);
