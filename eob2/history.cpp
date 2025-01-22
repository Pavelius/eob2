#include "bsdata.h"
#include "history.h"

historyi* find_history(short unsigned p1, short unsigned p2) {
	for(auto& e : bsdata<historyi>()) {
		if(e.p1 == p1 && e.p2 == p2)
			return &e;
	}
	return 0;
}

historyi* add_history(short unsigned p1, short unsigned p2) {
	if(p1 == 0xFFFF || p2 == 0xFFFF)
		return 0;
	auto p = find_history(p1, p2);
	if(!p) {
		p = bsdata<historyi>::add();
		memset(p, 0, sizeof(*p));
		p->p1 = p1;
		p->p2 = p2;
	}
	return p;
}

int get_history(short unsigned p1, short unsigned p2) {
	auto p = find_history(p1, p2);
	if(!p)
		return 0;
	return p->value;
}