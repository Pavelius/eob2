#pragma once

#include "nameable.h"
#include "variant.h"

struct listi : nameable {
	variants	elements;
};
extern listi* last_list;