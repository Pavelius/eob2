#pragma once

#include "nameable.h"
#include "party.h"
#include "picture.h"
#include "variant.h"

struct locationi : nameable {
	locationi*	parent;
	picturei	avatar;
	variants	options;
	char		required[Blessing + 1];
	const char*	getheader(const char* action) const;
};
extern locationi* last_location;