#include "bsdata.h"
#include "statable.h"

void statable::add(abilityn i, int v) {
	v += abilities[i];
	if(v < 0)
		v = 0;
	else if(v > 120)
		v = 120;
	abilities[i] = v;
}