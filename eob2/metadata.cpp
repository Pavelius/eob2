#include "ability.h"
#include "action.h"
#include "advancement.h"
#include "alignment.h"
#include "boost.h"
#include "bsreq.h"
#include "cell.h"
#include "class.h"
#include "condition.h"
#include "damage.h"
#include "dungeon.h"
#include "item.h"
#include "keyvalue.h"
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
#include "room.h"
#include "shop.h"
#include "spell.h"
#include "trap.h"
#include "variant.h"
#include "wearable.h"

NOBSDATA(color)
NOBSDATA(dice)
NOBSDATA(point)
NOBSDATA(picturei)
NOBSDATA(quest::leveli)

BSDATAC(actioni, 256)
BSDATAC(advancement, 256)
BSDATAC(boosti, 256)
BSDATAC(classi, 32)
BSDATAC(creaturei, 64)
BSDATAC(dungeoni, 256)
BSDATAC(itemi, 256)
BSDATAC(locationi, 128)
BSDATAC(monsteri, 256)
BSDATAC(quest, 128)
BSDATAC(racei, 16)
BSDATAC(randomeffecti, 128)
BSDATAC(roomi, 128)
BSDATAC(shopi, 32)
BSDATAC(spelli, 256)
BSDATAC(spellseta, 64)
BSDATAC(trapi, 64)

BSMETA(actioni) = {
	BSREQ(id),
	BSREQ(avatar),
	BSREQ(cost),
	BSFLG(races, racei),
	BSFLG(classes, classi),
	BSFLG(restrict_classes, classi),
	BSFLG(alignment, alignmenti),
	BSREQ(filter),
	BSREQ(filter_item),
	BSREQ(effect),
	BSDST(required, partystati),
	{}};
BSMETA(abilityi) = {
	BSREQ(id),
	{}};
BSMETA(advancement) = {
	BSREQ(type),
	BSREQ(id),
	BSREQ(elements),
	{}};
BSMETA(alignmenti) = {
	BSREQ(id),
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
	BSREQ(experience),
	BSREQ(non_player),
	BSENM(primary, abilityi),
	BSREQ(hd), BSREQ(exp_per_hd), BSREQ(caster),
	BSENM(classes, classi), BSREQ(count),
	BSFLG(alignment, alignmenti),
	BSFLG(races, racei),
	{}};
BSMETA(color) = {
	BSREQ(r), BSREQ(g), BSREQ(b),
	{}};
BSMETA(corridori) = {
	BSREQ(id),
	{}};
BSMETA(creaturei) = {
	BSDST(abilities, abilityi),
	BSDST(spells, spelli),
	{}};
BSMETA(dice) = {
	BSREQ(c), BSREQ(d), BSREQ(b), BSREQ(m),
	{}};
BSMETA(damagei) = {
	BSREQ(id),
	{}};
BSMETA(dungeoni) = {
	BSENM(quest_id, quest),
	{}};
BSMETA(feati) = {
	BSREQ(id),
	{}};
BSMETA(genderi) = {
	BSREQ(id),
	{}};
BSMETA(goali) = {
	BSREQ(id),
	{}};
BSMETA(itemi) = {
	BSREQ(id),
	BSENM(wear, weari),
	BSENM(damage_type, damagei),
	BSREQ(attack), BSREQ(number_attacks), BSREQ(speed),
	BSREQ(damage), BSREQ(damage_large),
	BSFLG(feats, feati),
	BSREQ(avatar), BSREQ(avatar_ground), BSREQ(avatar_thrown),
	BSREQ(chance_identify),
	BSREQ(ammo),
	BSREQ(wearing),
	BSREQ(cost),
	BSREQ(powers),
	{}};
BSMETA(locationi) = {
	BSREQ(id),
	BSREQ(parent),
	BSREQ(avatar),
	BSREQ(options),
	BSREQ(cost),
	BSDST(required, partystati),
	{}};
BSMETA(monsteri) = {
	BSREQ(id),
	BSREQ(experience),
	BSENM(res, residi),
	BSENM(alignment, alignmenti),
	BSENM(reaction, reactioni),
	BSREQ(frames), BSREQ(overlays),
	BSREQ(hd), BSREQ(ac),
	BSREQ(feats),
	BSREQ(minions),
	BSDST(spells, spelli),
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
	BSREQ(travel),
	BSREQ(reward), BSREQ(reward_history),
	BSREQ(difficult),
	BSDST(goals, goali),
	{}};
BSMETA(quest::leveli) = {
	BSENM(type, residi),
	BSENM(habbits, monsteri),
	BSENM(boss, monsteri),
	BSENM(minions, monsteri),
	BSREQ(level),
	BSREQ(webs), BSREQ(barrels), BSREQ(eggs), BSREQ(graves),
	BSREQ(blood), BSREQ(dirt), BSREQ(blades), BSREQ(jug),
	BSENM(key, itemi),
	BSENM(trap, trapi),
	BSENM(special, itemi),
	BSENM(language, racei),
	BSREQ(features),
	{}};
BSMETA(racei) = {
	BSREQ(id),
	BSREQ(minimal), BSREQ(maximal),
	BSFLG(languages, racei),
	BSFLG(specialization, itemi),
	BSENM(origin, racei),
	{}};
BSMETA(randomeffecti) = {
	BSREQ(id),
	BSREQ(base), BSREQ(raise), BSREQ(perlevel), BSREQ(multiplier),
	{}};
BSMETA(reactioni) = {
	BSREQ(id),
	{}};
BSMETA(residi) = {
	BSREQ(id),
	{}};
BSMETA(roomi) = {
	BSREQ(id),
	BSREQ(floor),
	BSREQ(shape),
	BSREQ(features),
	{}};
BSMETA(savei) = {
	BSREQ(id),
	{}};
BSMETA(shapei) = {
	BSREQ(id),
	{}};
BSMETA(shopi) = {
	BSREQ(id),
	BSREQ(effect),
	BSREQ(days),
	BSREQ(count),
	{}};
BSMETA(spelli) = {
	BSREQ(id),
	BSREQ(levels),
	BSREQ(lighting),
	BSREQ(avatar_thrown),
	BSFLG(feats, feati),
	BSREQ(summon),
	BSREQ(filter),
	BSREQ(filter_item),
	BSFLG(filter_cell, celli),
	BSREQ(instant),
	BSREQ(clearing),
	BSREQ(wearing),
	BSREQ(duration),
	{}};
BSMETA(trapi) = {
	BSREQ(id),
	BSENM(type, damagei),
	BSREQ(damage),
	BSREQ(projectile),
	BSENM(save, savei),
	BSREQ(avatar),
	BSREQ(targets),
	{}};
BSMETA(weari) = {
	BSREQ(id),
	{}};
BSDATA(varianti) = {
	{"NoVariant", VAR(varianti, 1)},
	{"Ability", VAR(abilityi, 1), 0, 0, ftscript<abilityi>},
	{"Action", VAR(actioni, 1), 0, 0, ftscript<actioni>},
	{"Advance", VAR(advancement, 2)},
	{"Alignment", VAR(alignmenti, 1)},
	{"Cell", VAR(celli, 1)},
	{"Class", VAR(classi, 1), 0, 0, ftscript<classi>},
	{"Condition", VAR(conditioni, 1), 0, 0, ftscript<conditioni>, fttest<conditioni>},
	{"Creature", VAR(creaturei, 0)},
	{"Damage", VAR(damagei, 1), 0, 0, ftscript<damagei>},
	{"Dungeon", VAR(dungeoni, 0)},
	{"Feat", VAR(feati, 1), 0, 0, ftscript<feati>, fttest<feati>},
	{"Formula", VAR(formulai, 1), 0, 0, ftscript<formulai>},
	{"Gender", VAR(genderi, 1), 0, 0, ftscript<genderi>},
	{"Item", VAR(itemi, 1), 0, 0, ftscript<itemi>},
	{"KeyValue", VAR(keyvaluei, 2)},
	{"List", VAR(listi, 1), 0, 0, ftscript<listi>},
	{"Location", VAR(locationi, 1), 0, 0, ftscript<locationi>},
	{"Modifier", VAR(modifieri, 1), 0, 0, ftscript<modifieri>},
	{"Monster", VAR(monsteri, 1)},
	{"Quest", VAR(quest, 1), 0, 0, ftscript<quest>},
	{"PartyAbility", VAR(partystati, 1), 0, 0, ftscript<partystati>},
	{"Race", VAR(racei, 1), 0, 0, ftscript<racei>},
	{"RandomEffect", VAR(randomeffecti, 1), 0, 0, ftscript<randomeffecti>},
	{"RandomList", VAR(randomizeri, 1), 0, 0, ftscript<randomizeri>},
	{"Reaction", VAR(reactioni, 1), 0, 0, ftscript<reactioni>},
	{"Room", VAR(roomi, 1)},
	{"Script", VAR(script, 1), 0, 0, ftscript<script>},
	{"Shape", VAR(shapei, 1), 0, 0, 0, 0, shape_read},
	{"Shop", VAR(shopi, 1), 0, 0, ftscript<shopi>, fttest<shopi>},
	{"Spell", VAR(spelli, 1), 0, 0, ftscript<spelli>, fttest<spelli>},
	{"Trap", VAR(trapi, 1)},
	{"Race", VAR(racei, 1)},
};
BSDATAF(varianti);
