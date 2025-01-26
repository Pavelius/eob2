#pragma once

#include "nameable.h"
#include "flagable.h"

enum alignmentn : unsigned char {
	LawfulGood, NeutralGood, ChaoticGood,
	LawfulNeutral, TrueNeutral, ChaoticNeutral,
	LawfulEvil, NeutralEvil, ChaoticEvil,
};
enum moralen : unsigned char {
	Neutral, Good, Evil, Lawful, Chaotic
};
typedef flagable<1, unsigned short> alignmenta;
typedef flagable<1, unsigned char> moralea;
struct alignmenti : nameable {
	alignmenta		similar;
	moralea			morale;
	bool			isallow(alignmentn v) const { return similar.is(v); }
};
extern alignmentn last_alignment;

bool ismorale(alignmentn v, moralen m);
