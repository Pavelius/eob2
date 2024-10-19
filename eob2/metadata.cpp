#include "ability.h"
#include "action.h"
#include "advancement.h"
#include "bsreq.h"
#include "cell.h"
#include "class.h"
#include "color.h"
#include "dungeon.h"
#include "item.h"
#include "list.h"
#include "location.h"
#include "feat.h"
#include "formula.h"
#include "gender.h"
#include "monster.h"
#include "shape.h"
#include "script.h"
#include "party.h"
#include "point.h"
#include "quest.h"
#include "randomizer.h"
#include "resid.h"
#include "race.h"
#include "randomeffect.h"
#include "spell.h"
#include "variant.h"
#include "wearable.h"

NOBSDATA(color)
NOBSDATA(dice)
NOBSDATA(dungeon_site)
NOBSDATA(point)
NOBSDATA(picturei)

BSDATAC(actioni, 256)
BSDATAC(advancement, 256)
BSDATAC(classi, 32)
BSDATAC(creaturei, 64)
BSDATAC(dungeoni, 256)
BSDATAC(itemi, 256)
BSDATAC(locationi, 128)
BSDATAC(monsteri, 256)
BSDATAC(quest, 128)
BSDATAC(racei, 16)
BSDATAC(randomeffecti, 128)
BSDATAC(spelli, 256)

BSMETA(actioni) = {
	BSREQ(id),
	BSREQ(avatar),
	BSFLG(races, racei),
	BSFLG(classes, classi),
	BSREQ(effect),
	{}};
BSMETA(abilityi) = {
	BSREQ(id),
	{}};
BSMETA(advancement) = {
	BSREQ(type),
	BSREQ(id),
	BSREQ(elements),
	{}};
BSMETA(celli) = {
	BSREQ(id),
	BSENM(res, residi),
	BSREQ(frame),
	BSFLG(flags, cellfi),
	{}};
BSMETA(cellfi) = {
	BSREQ(id),
	{}};
BSMETA(classi) = {
	BSREQ(id),
	BSREQ(minimal),
	BSENM(primary, abilityi),
	BSREQ(hd),
	BSENM(classes, classi), BSREQ(count),
	{}};
BSMETA(creaturei) = {
	BSDST(abilities, abilityi),
	BSDST(spells, spelli),
	{}};
BSMETA(dice) = {
	BSREQ(c), BSREQ(d), BSREQ(b), BSREQ(m),
	{}};
BSMETA(dungeoni) = {
	BSENM(quest_id, quest),
	{}};
BSMETA(dungeon_site) = {
	BSENM(type, residi),
	BSENM(habbits, monsteri),
	BSREQ(level),
	BSENM(key, itemi),
	BSENM(special, itemi),
	BSENM(language, racei),
	{}};
BSMETA(formulai) = {
	BSREQ(id),
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
	BSREQ(avatar), BSREQ(avatar_ground),
	BSREQ(ammo),
	BSREQ(wearing), BSREQ(use),
	BSREQ(cost),
	{}};
BSMETA(genderi) = {
	BSREQ(id),
	{}};
BSMETA(locationi) = {
	BSREQ(id),
	BSREQ(parent),
	BSREQ(avatar),
	BSREQ(options),
	{}};
BSMETA(monsteri) = {
	BSREQ(id),
	BSENM(res, residi),
	BSREQ(frames), BSREQ(overlays),
	BSREQ(hd),
	BSREQ(feats),
	{}};
BSMETA(partystati) = {
	BSREQ(id),
	{}};
BSMETA(picturei) = {
	BSENM(res, residi),
	BSREQ(frame),
	{}};
BSMETA(quest) = {
	BSREQ(id),
	BSREQ(sites),
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
BSMETA(shapei) = {
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
	{"Action", VAR(actioni, 1), 0, 0, ftscript<actioni>},
	{"Advance", VAR(advancement, 2)},
	{"Cell", VAR(celli, 1)},
	{"Class", VAR(classi, 1), 0, 0, ftscript<classi>},
	{"Creature", VAR(creaturei, 0)},
	{"Dungeon", VAR(dungeoni, 0)},
	{"Feat", VAR(feati, 1), 0, 0, ftscript<feati>},
	{"Formula", VAR(formulai, 1), 0, 0, ftscript<formulai>},
	{"Gender", VAR(genderi, 1), 0, 0, ftscript<genderi>},
	{"Item", VAR(itemi, 1), 0, 0, ftscript<itemi>},
	{"List", VAR(listi, 1), 0, 0, ftscript<listi>},
	{"Location", VAR(locationi, 1), 0, 0, ftscript<locationi>},
	{"Modifier", VAR(modifieri, 1), 0, 0, ftscript<modifieri>},
	{"Monster", VAR(monsteri, 1)},
	{"Quest", VAR(quest, 1), 0, 0, ftscript<quest>},
	{"PartyAbility", VAR(partystati, 1), 0, 0, ftscript<partystati>},
	{"Race", VAR(racei, 1), 0, 0, ftscript<racei>},
	{"RandomEffect", VAR(randomeffecti, 1)},
	{"RandomList", VAR(randomizeri, 1), 0, 0, ftscript<randomizeri>},
	{"Script", VAR(script, 1), 0, 0, ftscript<script>},
	{"Shape", VAR(shapei, 1), 0, 0, 0, 0, shape_read},
	{"Spell", VAR(spelli, 1)},
	{"Race", VAR(racei, 1)},
};
BSDATAF(varianti);
