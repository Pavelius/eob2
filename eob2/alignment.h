#pragma once

#include "nameable.h"
#include "flagable.h"

enum alignmentn : unsigned char {
	LawfulGood, NeutralGood, ChaoticGood,
	LawfulNeutral, TrueNeutral, ChaoticNeutral,
	LawfulEvil, NeutralEvil, ChaoticEvil,
};
struct alignmenti : nameable {
};
extern alignmentn last_alignment;

typedef flagable<1, unsigned short> alignmenta;
