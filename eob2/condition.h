#pragma once

#include "variant.h"

struct conditioni : nameable {
	typedef bool(*fntest)();
	fntest proc;
};
