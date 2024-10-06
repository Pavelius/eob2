#include "bsdata.h"
#include "gender.h"

BSDATA(genderi) = {
	{"NoGender"},
	{"Male"},
	{"Female"},
};
assert_enum(genderi, Female)

gendern last_gender;