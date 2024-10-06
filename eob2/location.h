#pragma once

#include "nameable.h"
#include "picture.h"
#include "variant.h"

struct locationi : nameable {
	picturei avatar;
	variants options;
};
extern locationi* last_location;