#include "adat.h"
#include "bsdata.h"
#include "item.h"
#include "list.h"
#include "math.h"
#include "rand.h"
#include "wearable.h"

static_assert(sizeof(item) == 4, "Size of item structure must be 4 bytes");

item* last_item;

void item::clear() {
	memset(this, 0, sizeof(*this));
}

const itemi& item::geti() const {
	return bsdata<itemi>::elements[type];
}

void item::setcount(int v) {
	if(!v)
		clear();
	else if(iscountable())
		count = v - 1;
}

void item::setpower(variant v) {
	power = 0;
	auto& ei = geti();
	if(!ei.powers)
		return;
	auto index = 0;
	for(auto e : ei.powers->elements) {
		if(e == v) {
			power = index;
			return;
		}
		index++;
	}
}

bool item::iscountable() const {
	return geti().wear == Quiver;
}

bool item::isallow(wearn v) const {
	auto n = geti().wear;
	switch(v) {
	case LeftRing:
	case RightRing:
		return n == LeftRing
			|| n == RightRing;
	case RightHand:
		return n == LeftHand
			|| n == RightHand
			|| n == Rod
			|| n == Readable
			|| n == Drinkable;
	case FirstBelt: case SecondBelt: case LastBelt:
		switch(n) {
		case LeftHand:
		case RightHand:
		case Drinkable:
			return true;
		default:
			return false;
		}
	default:
		if(v >= Backpack && v <= LastBackpack)
			return true;
		return n == v;
	}
}

void item::create(const itemi* pi) {
	clear();
	if(!pi)
		return;
	create(pi - bsdata<itemi>::elements);
}

void item::create(int value) {
	type = value;
	flags = 0;
	power = 0;
	count = 0;
	if(iscountable())
		count += xrand(0, 4);
}

const char*	item::getname() const {
	static char temp[32];
	stringbuilder sb(temp);
	sb.clear();
	getname(sb);
	return temp;
}

void item::getname(stringbuilder& sb) const {
	if(isdamaged())
		sb.adds(getnm((geti().wear == Edible) ? "Rotten" : "Damaged"));
	if(identified) {
		if(cursed)
			sb.adds(getnm("Cursed"));
	}
	sb.adds(bsdata<itemi>::elements[type].getname());
	if(identified) {
		auto power = getpower();
		if(power) {
			auto pn = getnme(str("Of%1%2i", power.getid(), iabs(power.counter)));
			if(!pn)
				pn = getnme(ids("Of", power.getid()));
			if(pn) {
				sb.adds("of ");
				sb.add(pn, power.counter);
			} else
				sb.add("%+1i", power.counter);
		}
	}
}

int	item::getcost() const {
	return geti().cost;
}

bool item::isweapon() const {
	return geti().damage.c != 0;
}

void item::damage(int bonus) {
	if(bonus >= 0) {
		bonus += count;
		if(bonus >= 10) {
			if(isartifact())
				count = 9;
			else
				clear();
		} else
			count = bonus;
	} else
		count = 0;
}

bool item::join(item& it) {
	if(type != it.type || flags != it.flags || power != it.power)
		return false;
	if(!iscountable())
		return false;
	auto v = getcount() + it.getcount();
	if(v <= 256) {
		setcount(v);
		it.clear();
		return true;
	} else {
		setcount(256);
		it.setcount(v - 256);
		return false;
	}
}

variant item::getpower() const {
	auto pi = geti().powers;
	if(!pi)
		return variant();
	return pi->elements.begin()[power];
}

void item::createpower(char magic_bonus, int chance_magical, int chance_cursed) {
	auto& ei = geti();
	if(!ei.powers)
		return;
	if(chance_cursed > 0) {
		if(d100() < chance_cursed)
			curse(1);
	}
	if(!ei.powers->elements.begin()[0]) {
		// Item may be non-magical if first power is empthy
		if(d100() >= chance_magical)
			return;
	}
	adat<variant, 32> source;
	while(magic_bonus >= 0) {
		source.clear();
		for(auto v : ei.powers->elements) {
			if(!v)
				continue;
			if(v.counter == magic_bonus)
				source.add(v);
		}
		if(source) {
			zshuffle(source.data, source.count);
			setpower(source.data[0]);
			return;
		}
		magic_bonus--;
	}
}

void item::usecharge() {
	if(d100() < 36)
		return;
	count++;
	if(count > 10)
		clear();
}