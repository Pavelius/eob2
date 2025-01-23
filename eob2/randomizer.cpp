/////////////////////////////////////////////////////////////////////////
// 
// Copyright 2024 Pavel Chistyakov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http ://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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
		else
			break;
	}
	return v;
}

template<> void ftscript<randomizeri>(int index, int bonus) {
	script_run(single(bsdata<randomizeri>::elements + index));
}