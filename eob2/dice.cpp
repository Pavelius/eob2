#include "dice.h"
#include "rand.h"

int dice::roll() const {
	if(!d)
		return c;
	int r = b;
	for(auto i = c; i > 0; i--)
		r += 1 + rand() % d;
	if(m)
		r *= m;
	return r;
}

int dice::maximum() const {
	if(!d)
		return c;
	return c * d + b;
}

int dice::minimum() const {
	return c + b;
}