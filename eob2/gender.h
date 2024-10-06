#pragma once

#include "nameable.h"

enum gendern : unsigned char {
	NoGender, Male, Female,
};
struct genderi : nameable {
};
extern gendern last_gender;
