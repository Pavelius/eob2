#include "formula.h"
#include "randomeffect.h"
#include "script.h"

randomeffecti* last_effect;
int last_level;

int randomeffecti::roll(int level) const {
	auto result = base.roll();
	auto start_level = perlevel[0];
	auto per_level = perlevel[1];
	auto cap_level = perlevel[2];
	if(cap_level && level > cap_level)
		level = cap_level;
	if(raise && per_level && level > start_level) {
		auto count = (level - start_level) / per_level;
		for(auto i = 0; i < count; i++)
			result += raise.roll();
	}
	if(multiplier)
		result *= multiplier;
	return result;
}

template<> void ftscript<randomeffecti>(int value, int bonus) {
	last_effect = bsdata<randomeffecti>::elements + value;
	last_number = bsdata<randomeffecti>::elements[value].roll(last_level) + bonus;
}