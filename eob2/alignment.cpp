#include "alignment.h"
#include "bsdata.h"

BSDATA(alignmenti) = {
	{"LawfulGood"},
	{"NeutralGood"},
	{"ChaoticGood"},
	{"LawfulNeutral"},
	{"TrueNeutral"},
	{"ChaoticNeutral"},
	{"LawfulEvil"},
	{"NeutralEvil"},
	{"ChaoticEvil"},
};
assert_enum(alignmenti, ChaoticEvil)

alignmentn last_alignment;