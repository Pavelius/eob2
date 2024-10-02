#pragma once

#include "adat.h"
#include "feat.h"
#include "nameable.h"

enum abilityn : unsigned char;
typedef char abilitya[6];

struct classi : nameable, featable {
	adat<char, 3> classes;
	int			hd;
	featable	wears;
	abilitya	minimal;
	abilityn	primary;
};