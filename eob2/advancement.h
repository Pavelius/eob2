#include "variant.h"

#pragma once

struct advancement {
	const char*	id;
	variant		type;
	variants	elements;
};