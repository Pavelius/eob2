#include "bsdata.h"
#include "wearable.h"

BSDATA(weari) = {
	{"Backpack"},
	{"Edible"},
	{"Drinkable"},
	{"Readable"},
	{"Usable"},
	{"Key"},
	{"Rod"},
	{"Backpack7"},
	{"Backpack8"},
	{"Backpack9"},
	{"Backpack10"},
	{"Backpack11"},
	{"Backpack12"},
	{"Backpack13"},
	{"Head"},
	{"Neck"},
	{"Body"},
	{"RightHand"},
	{"LeftHand"},
	{"RightRing"},
	{"LeftRing"},
	{"Elbow"},
	{"Legs"},
	{"Quiver"},
	{"FirstBelt"},
	{"SecondBelt"},
	{"LastBelt"},
};
assert_enum(weari, LastBelt)

void wearable::equip(item& v) {
	for(auto i = Head; i <= LastBelt; i = (wearn)(i + 1)) {
		if(!v.isallow(i))
			continue;
		last_item = &wears[i];
		if(v.iscountable()) {
			if(!wears[i].join(v))
				continue;
		} else {
			if(wears[i])
				continue;
			wears[i] = v;
			v.clear();
		}
		break;
	}
}

void wearable::additem(item& v) {
	if(v.iscountable()) {
		for(auto& e : backpack()) {
			if(!e.join(v))
				continue;
			last_item = &e;
			return;
		}
	}
	for(auto& e : backpack()) {
		if(e)
			continue;
		e = v;
		last_item = &e;
		v.clear();
		break;
	}
}

item* wearable::freebelt() {
	for(auto& e : beltslots()) {
		if(!e)
			return &e;
	}
	return 0;
}

item* wearable::freebackpack() {
	for(auto& e : backpack()) {
		if(!e)
			return &e;
	}
	return 0;
}