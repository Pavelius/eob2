#include "bsreq.h"
#include "list.h"

BSMETA(listi) = {
	BSREQ(id),
	BSREQ(elements),
	{}};
BSDATAC(listi, 256)

listi*	last_list;

void script_run(const variants& source);

void listi::run() const {
	auto push_list = last_list;
	last_list = (listi*)this;
	script_run(elements);
	last_list = push_list;
}

void run_command(const char* id) {
	auto p = bsdata<listi>::find(id);
	if(p)
		p->run();
}