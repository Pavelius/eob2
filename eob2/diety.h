#pragma once

#include "alignment.h"
#include "nameable.h"
#include "variant.h"

enum dietyn : unsigned char;

struct dietyi : nameable {
	alignmentn	alignment;
	variants	minor, major;
};
