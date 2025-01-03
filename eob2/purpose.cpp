#include "bsdata.h"
#include "purpose.h"

BSDATA(purposei) = {
	{"CommonItem"},
	{"SummonedItem"},
	{"ToolItem"},
	{"QuestItem"},
	{"NaturalItem"},
};
assert_enum(purposei, NaturalItem)