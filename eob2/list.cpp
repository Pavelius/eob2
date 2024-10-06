#include "bsreq.h"
#include "list.h"
#include "script.h"

BSMETA(listi) = {
	BSREQ(id),
	BSREQ(elements),
	{}};
BSDATAC(listi, 256)

listi* last_list;

void script_run(const variants& source);

template<> void ftscript<listi>(int value, int counter) {
	auto push_list = last_list;
	last_list = bsdata<listi>::elements + value;
	script_run(last_list->elements);
	last_list = push_list;
}