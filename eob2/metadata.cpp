#include "ability.h"
#include "advancement.h"
#include "bsreq.h"
#include "class.h"
#include "color.h"
#include "item.h"
#include "feat.h"
#include "script.h"
#include "point.h"
#include "race.h"
#include "variant.h"

NOBSDATA(color)
NOBSDATA(dice)
NOBSDATA(point)

BSDATAC(advancement, 256)
BSDATAC(classi, 32)
BSDATAC(itemi, 256)
BSDATAC(racei, 16)

BSMETA(abilityi) = {
	BSREQ(id),
	{}};
BSMETA(advancement) = {
	BSREQ(type),
	BSREQ(id),
	BSREQ(elements),
	{}};
BSMETA(classi) = {
	BSREQ(id),
	BSREQ(minimal),
	BSENM(primary, abilityi),
	BSREQ(hd),
	BSENM(classes, classi), BSREQ(count),
	{}};
BSMETA(damagei) = {
	BSREQ(id),
	{}};
BSMETA(dice) = {
	BSREQ(c), BSREQ(d), BSREQ(b), BSREQ(m),
	{}};
BSMETA(feati) = {
	BSREQ(id),
	{}};
BSMETA(itemi) = {
	BSREQ(id),
	BSENM(harm, damagei),
	BSREQ(attack), BSREQ(number_attacks),
	BSREQ(damage), BSREQ(damage_large),
	BSFLG(feats, feati),
	BSREQ(ammo),
	{}};
BSMETA(racei) = {
	BSREQ(id),
	BSREQ(minimal), BSREQ(maximal),
	{}};
BSDATA(varianti) = {
	{"NoVariant"},
	{"Ability", VAR(abilityi, 1), 0, 0, ftscript<abilityi>},
	{"Advance", VAR(advancement, 2)},
	{"Class", VAR(classi, 1)},
	{"Item", VAR(itemi, 1)},
	{"Feat", VAR(feati, 1), 0, 0, ftscript<feati>},
	{"Modifier", VAR(modifieri, 1), 0, 0, ftscript<modifieri>},
	{"Script", VAR(script, 1), 0, 0, ftscript<script>},
	{"Race", VAR(racei, 1)},
};
BSDATAF(varianti);