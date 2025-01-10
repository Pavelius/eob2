#include "alignment.h"
#include "bsdata.h"
#include "math.h"

BSDATA(alignmenti) = {
	{"LawfulGood", {FG(LawfulGood) | FG(NeutralGood)}},
	{"NeutralGood", {FG(LawfulGood) | FG(NeutralGood) | FG(ChaoticGood)}},
	{"ChaoticGood", {FG(NeutralGood) | FG(ChaoticGood)}},
	{"LawfulNeutral", {FG(LawfulNeutral) | FG(TrueNeutral) | FG(NeutralGood)}},
	{"TrueNeutral", {FG(NeutralGood) | FG(TrueNeutral) | FG(NeutralEvil)}},
	{"ChaoticNeutral", {FG(ChaoticGood) | FG(ChaoticNeutral) | FG(ChaoticEvil)}},
	{"LawfulEvil", {FG(LawfulEvil) | FG(NeutralEvil)}},
	{"NeutralEvil", {FG(LawfulEvil) | FG(NeutralEvil) | FG(ChaoticEvil)}},
	{"ChaoticEvil", {FG(NeutralEvil) | FG(ChaoticEvil)}},
};
assert_enum(alignmenti, ChaoticEvil)

alignmentn last_alignment;