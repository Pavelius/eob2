#pragma once

#include "ability.h"
#include "nameable.h"
#include "picture.h"
#include "variant.h"

struct carousingi : nameable {
	abilityn	ability;
	picturei	avatar;
	variants	filter, fail, success;
};
extern carousingi* last_carousing;

void apply_carousing(int bonus);
