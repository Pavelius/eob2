#include "bsreq.h"
#include "list.h"
#include "script.h"

BSMETA(listi) = {
	BSREQ(id),
	BSREQ(elements),
	{}};
BSDATAC(listi, 256)

template<> void ftscript<listi>(int value, int counter) {
	auto p = bsdata<listi>::elements + value;
	script_run(p->id, p->elements);
}