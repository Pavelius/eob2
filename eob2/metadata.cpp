#include "ability.h"
#include "bsreq.h"
#include "class.h"
#include "color.h"
#include "point.h"
#include "race.h"
#include "variant.h"

NOBSDATA(color)
NOBSDATA(point)

BSDATAC(classi, 32)
BSDATAC(racei, 16)

BSMETA(abilityi) = {
	BSREQ(id),
	{}};
BSMETA(classi) = {
	BSREQ(id),
	BSCST(classes, KindADat, classn, classi, 3),
	BSFLG(feats, feati),
	BSFLG(wears, feati),
	BSREQ(minimal),
	BSENM(primary, abilityi),
	{}};
BSMETA(feati) = {
	BSREQ(id),
	{}};
BSMETA(racei) = {
	BSREQ(id),
	BSREQ(minimal), BSREQ(maximal),
	{}};
BSDATA(varianti) = {
	{"NoVariant"},
	{"Class", VAR(classi, 1)},
	{"Race", VAR(racei, 1)},
};
BSDATAF(varianti);