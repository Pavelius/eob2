#include "bsreq.h"
#include "condition.h"
#include "script.h"

BSMETA(conditioni) = {
	BSREQ(id),
	{}};

template<> bool fttest<conditioni>(int index, int value) {
	if(value>=0)
		return bsdata<conditioni>::elements[index].proc();
	else
		return !bsdata<conditioni>::elements[index].proc();
}

template<> void ftscript<conditioni>(int index, int value) {
}