#include "bsreq.h"
#include "color.h"
#include "point.h"
#include "race.h"
#include "variant.h"

NOBSDATA(color)
NOBSDATA(point)
BSDATAC(racei, 16)

BSMETA(racei) = {
	BSREQ(id),
	BSREQ(minimal), BSREQ(maximal),
	{}};
BSDATA(varianti) = {
	{"NoVariant"},
	{"Race", VAR(racei, 1)},
};
BSDATAF(varianti);