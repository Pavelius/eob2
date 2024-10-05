#include "avatar.h"
#include "bsdata.h"
#include "console.h"
#include "gender.h"
#include "groupname.h"
#include "npc.h"
#include "race.h"
#include "stringbuilder.h"

unsigned short generate_name(int race, gendern gender) {
	auto pr = bsdata<racei>::elements + race;
	auto pg = bsdata<genderi>::elements + gender;
	unsigned short name = 0xFFFF;
	if(name == 0xFFFF)
		name = random_group_namei(ids(pr->id, pg->id));
	if(name == 0xFFFF && szstart(pr->id, "Half"))
		name = random_group_namei(ids(pr->id + 4, pg->id));
	return name;
}

const char*	npc::getname() const {
	if(name == 0xFFFF)
		return "Noname";
	return get_group_name(name);
}

void npc::say(const char* format, ...) const {
	sayv(format, xva_start(format));
}

void npc::sayv(const char* format, const char* format_param) const {
	consolenl();
	console("%1 \"", getname());
	consolev(format, format_param);
	console("\"");
}