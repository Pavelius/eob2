#include "bsdata.h"
#include "boost.h"

void clear_boost(unsigned stamp, fnclearboost proc) {
	auto ps = bsdata<boosti>::begin();
	for(auto& e : bsdata<boosti>()) {
		if(e.stamp <= stamp) {
			if(proc)
				proc(e.target, e.type, e.param);
			continue;
		}
		*ps++ = e;
	}
	bsdata<boosti>::source.count = ps - bsdata<boosti>::elements;
}

boosti* find_boost(referencei target, short type, short param) {
	for(auto& e : bsdata<boosti>()) {
		if(e.type == type && e.param == param && e.target == target)
			return &e;
	}
	return 0;
}

void add_boost(unsigned stamp, referencei target, short type, short param) {
	auto p = find_boost(target, type, param);
	if(p) {
		if(p->stamp < stamp)
			p->stamp = stamp;
		return;
	}
	p = bsdata<boosti>::add();
	p->target = target;
	p->type = type;
	p->param = param;
	p->stamp = stamp;
}