#include "ability.h"
#include "advancement.h"
#include "bsreq.h"
#include "class.h"
#include "color.h"
#include "item.h"
#include "list.h"
#include "location.h"
#include "feat.h"
#include "gender.h"
#include "script.h"
#include "party.h"
#include "point.h"
#include "resid.h"
#include "race.h"
#include "randomeffect.h"
#include "spell.h"
#include "variant.h"
#include "wearable.h"

NOBSDATA(color)
NOBSDATA(dice)
NOBSDATA(point)
NOBSDATA(picturei)

BSDATAC(advancement, 256)
BSDATAC(classi, 32)
BSDATAC(itemi, 256)
BSDATAC(locationi, 128)
BSDATAC(racei, 16)
BSDATAC(randomeffecti, 128)
BSDATAC(spelli, 256)

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
BSMETA(dice) = {
	BSREQ(c), BSREQ(d), BSREQ(b), BSREQ(m),
	{}};
BSMETA(feati) = {
	BSREQ(id),
	{}};
BSMETA(itemi) = {
	BSREQ(id),
	BSENM(wear, weari),
	BSREQ(attack), BSREQ(number_attacks), BSREQ(speed),
	BSREQ(damage), BSREQ(damage_large),
	BSFLG(feats, feati),
	BSREQ(avatar),
	BSREQ(ammo),
	BSREQ(wearing), BSREQ(use),
	BSREQ(cost),
	{}};
BSMETA(genderi) = {
	BSREQ(id),
	{}};
BSMETA(locationi) = {
	BSREQ(id),
	BSREQ(group),
	BSREQ(avatar),
	BSREQ(options),
	{}};
BSMETA(partystati) = {
	BSREQ(id),
	{}};
BSMETA(picturei) = {
	BSENM(id, residi),
	BSREQ(frame),
	{}};
BSMETA(racei) = {
	BSREQ(id),
	BSREQ(minimal), BSREQ(maximal),
	{}};
BSMETA(randomeffecti) = {
	BSREQ(id),
	BSREQ(base), BSREQ(raise), BSREQ(perlevel), BSREQ(multiplier),
	{}};
BSMETA(residi) = {
	BSREQ(id),
	{}};
BSMETA(spelli) = {
	BSREQ(id),
	BSREQ(levels),
	BSREQ(effect), BSREQ(duration),
	BSFLG(feats, feati),
	BSREQ(summon), BSREQ(wearing),
	{}};
BSMETA(weari) = {
	BSREQ(id),
	{}};
BSDATA(varianti) = {
	{"NoVariant"},
	{"Ability", VAR(abilityi, 1), 0, 0, ftscript<abilityi>},
	{"Advance", VAR(advancement, 2)},
	{"Class", VAR(classi, 1), 0, 0, ftscript<classi>},
	{"Feat", VAR(feati, 1), 0, 0, ftscript<feati>},
	{"Gender", VAR(genderi, 1), 0, 0, ftscript<genderi>},
	{"Item", VAR(itemi, 1), 0, 0, ftscript<itemi>},
	{"List", VAR(listi, 1), 0, 0, ftscript<listi>},
	{"Location", VAR(locationi, 1), 0, 0, ftscript<locationi>},
	{"Modifier", VAR(modifieri, 1), 0, 0, ftscript<modifieri>},
	{"PartyAbility", VAR(partystati, 1), 0, 0, ftscript<partystati>},
	{"Race", VAR(racei, 1), 0, 0, ftscript<racei>},
	{"RandomEffect", VAR(randomeffecti, 1)},
	{"Script", VAR(script, 1), 0, 0, ftscript<script>},
	{"Spell", VAR(spelli, 1)},
	{"Race", VAR(racei, 1)},
};
BSDATAF(varianti);