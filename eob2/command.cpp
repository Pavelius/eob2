#include "bsreq.h"
#include "command.h"
#include "script.h"

BSMETA(commandi) = {
	BSREQ(id),
	{}};

template<> void ftscript<commandi>(int value, int bonus) {
	bsdata<commandi>::elements[value].proc();
}