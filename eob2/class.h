#pragma once

#include "adat.h"
#include "feat.h"
#include "nameable.h"

enum classn : unsigned char;

struct classi : nameable, featable {
	adat<classn, 3> classes;
	featable		wears;
};