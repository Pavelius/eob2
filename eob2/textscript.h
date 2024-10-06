#pragma once

#include "stringbuilder.h"

struct textscript {
	const char*	id;
	fnprint		proc;
};
void initialize_strings();
