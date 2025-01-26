#include "alignment.h"
#include "bsdata.h"
#include "math.h"

BSDATA(alignmenti) = {
	{"LawfulGood", {FG(LawfulGood) | FG(NeutralGood)}, {FG(Lawful) | FG(Good)}},
	{"NeutralGood", {FG(LawfulGood) | FG(NeutralGood) | FG(ChaoticGood)}, {FG(Neutral) | FG(Good)}},
	{"ChaoticGood", {FG(NeutralGood) | FG(ChaoticGood)}, {FG(Chaotic) | FG(Good)}},
	{"LawfulNeutral", {FG(LawfulNeutral) | FG(TrueNeutral) | FG(NeutralGood)}, {FG(Lawful) | FG(Neutral)}},
	{"TrueNeutral", {FG(NeutralGood) | FG(TrueNeutral) | FG(NeutralEvil)}, {FG(Neutral)}},
	{"ChaoticNeutral", {FG(ChaoticGood) | FG(ChaoticNeutral) | FG(ChaoticEvil)}, {FG(Chaotic) | FG(Neutral)}},
	{"LawfulEvil", {FG(LawfulEvil) | FG(NeutralEvil)}, {FG(Lawful) | FG(Evil)}},
	{"NeutralEvil", {FG(LawfulEvil) | FG(NeutralEvil) | FG(ChaoticEvil)}, {FG(Neutral) | FG(Evil)}},
	{"ChaoticEvil", {FG(NeutralEvil) | FG(ChaoticEvil)}, {FG(Chaotic) | FG(Evil)}},
};
assert_enum(alignmenti, ChaoticEvil)

alignmentn last_alignment;

bool ismorale(alignmentn v, moralen m) {
	return bsdata<alignmenti>::elements[v].morale.is(m);
}