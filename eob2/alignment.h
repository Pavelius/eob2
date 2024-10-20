#pragma once

#include "nameable.h"

enum alignmentn : unsigned char {
	LawfulGood, NeutralGood, ChaoticGood,
	LawfulNeutral, TrueNeutral, ChaoticNeutral,
	LawfulEvil, NeutralEvil, ChaoticEvil,
};
struct alignmenti : nameable {
};
extern alignmentn last_alignment;