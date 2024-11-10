#include "boost.h"

void clear_boost(unsigned stamp, fnclearboost proc) {
	auto ps = bsdata<boosti>::begin();
	for(auto& e : bsdata<boosti>()) {
		if(e.stamp <= stamp) {
			proc(e.target, e.effect);
			continue;
		}
		*ps++ = e;
	}
	bsdata<boosti>::source.count = ps - bsdata<boosti>::elements;
}

boosti* find_boost(referencei target, variant effect) {
	for(auto& e : bsdata<boosti>()) {
		if(e.target == target && e.effect == effect)
			return &e;
	}
	return 0;
}

void add_boost(unsigned stamp, referencei target, variant effect) {
	auto p = find_boost(target, effect);
	if(p) {
		if(p->stamp < stamp)
			p->stamp = stamp;
		return;
	}
	p = bsdata<boosti>::add();
	p->target = target;
	p->effect = effect;
	p->stamp = stamp;
}