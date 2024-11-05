#include "bsreq.h"
#include "list.h"
#include "rand.h"
#include "randomizer.h"
#include "script.h"

BSMETA(randomizeri) = {
	BSREQ(id),
	BSREQ(chance),
	{}};
BSDATAC(randomizeri, 512)

static int getcounter(variant v) {
	return (v.counter < 1) ? 1 : v.counter;
}

variant	randomizeri::random() const {
	return random_variant(chance);
}

int random_total(const variants& elements) {
	auto result = 0;
	for(auto& e : elements)
		result += getcounter(e);
	return result;
}

variant random_variant(const variants& elements) {
	variant lr = variant();
	auto summary = random_total(elements);
	if(summary) {
		auto result = (rand() % summary);
		for(auto& e : elements) {
			auto n = getcounter(e);
			lr = e; lr.counter = 0;
			if(result < n)
				break;
			result -= n;
		}
	}
	return lr;
}

variant random_equal(const variants& elements) {
	if(!elements)
		return variant();
	return elements.begin()[rand() % elements.size()];
}

variant single(variant v) {
	while(true) {
		if(v.iskind<randomizeri>())
			v = random_variant(bsdata<randomizeri>::elements[v.value].chance);
		//else if(v.iskind<listi>())
		//	v = random_equal(bsdata<listi>::elements[v.value].elements);
		else
			break;
	}
	return v;
}

template<> void ftscript<randomizeri>(int index, int bonus) {
	script_run(single(bsdata<randomizeri>::elements + index));
}