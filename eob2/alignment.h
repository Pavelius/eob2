#pragma once

#include "nameable.h"
#include "flagable.h"

enum alignmentn : unsigned char {
	LawfulGood, NeutralGood, ChaoticGood,
	LawfulNeutral, TrueNeutral, ChaoticNeutral,
	LawfulEvil, NeutralEvil, ChaoticEvil,
};
typedef flagable<1, unsigned short> alignmenta;
struct alignmenti : nameable {
	alignmenta		similar;
	bool			isallow(alignmentn v) const { return similar.is(v); }
};
extern alignmentn last_alignment;
