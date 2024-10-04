#include "bsdata.h"
#include "wearable.h"

BSDATA(weari) = {
	{"Backpack"},
	{"Backpack1"},
	{"Backpack2"},
	{"Backpack3"},
	{"Backpack4"},
	{"Backpack5"},
	{"Backpack6"},
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
		if(wears[i])
			continue;
		if(!v.isallow(i))
			continue;
		wears[i] = v;
		last_item = &wears[i];
		v.clear();
		break;
	}
}

void wearable::additem(item& v) {
	for(auto& e : backpack()) {
		if(e)
			continue;
		e = v;
		last_item = &e;
		v.clear();
		break;
	}
}