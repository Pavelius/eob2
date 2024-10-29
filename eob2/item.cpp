#include "bsdata.h"
#include "item.h"
#include "list.h"
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
			|| n == RightHand;
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
	if(isdamaged())
		sb.adds(getnm(is(Disease) ? "Rotten" : "Damaged"));
	if(identified) {
		if(cursed)
			sb.adds(getnm("Cursed"));
	}
	sb.adds(bsdata<itemi>::elements[type].getname());
	return temp;
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
		if(bonus >= 10)
			clear();
		else
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