#pragma once

#include "alignment.h"
#include "nameable.h"
#include "variant.h"

struct dietyi : nameable {
	alignmentn	alignment;
	variants	minor, major;
};
extern dietyi* last_diety;