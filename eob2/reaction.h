#pragma once

#include "nameable.h"

enum reactions : unsigned char {
	Indifferent, Friendly, Careful, Hostile,
};
extern reactions last_reaction;
struct reactioni : nameable {
};