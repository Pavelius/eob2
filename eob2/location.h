#pragma once

#include "nameable.h"
#include "picture.h"
#include "variant.h"

struct locationi : nameable {
	const char* group;
	picturei avatar;
	variants options;
	const char*	getheader(const char* action) const;
	const char*	getname() const;
};
extern locationi* last_location;