#pragma once

#include "adat.h"
#include "feat.h"
#include "nameable.h"

enum classn : unsigned char;
enum abilityn : unsigned char;
typedef char abilitya[6];

struct classi : nameable, featable {
	adat<classn, 3> classes;
	featable	wears;
	abilitya	minimal;
	abilityn	primary;
};