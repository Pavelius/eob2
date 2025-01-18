#pragma once

#include "alignment.h"
#include "nameable.h"
#include "variant.h"

struct dietyi : nameable {
	alignmentn	alignment;
	variants	powers;
};
extern dietyi* last_diety;