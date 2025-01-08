#include "avatar.h"
#include "alignment.h"
#include "bsdata.h"
#include "console.h"
#include "class.h"
#include "gender.h"
#include "speech.h"
#include "item.h"
#include "npc.h"
#include "race.h"
#include "stringbuilder.h"

static unsigned short generate_name(racen race, gendern gender, fnallowus name_filter) {
	auto pr = bsdata<racei>::elements + race;
	auto pg = bsdata<genderi>::elements + gender;
	unsigned short name = 0xFFFF;
	if(name == 0xFFFF)
		name = speech_random_name(ids(pr->id, pg->id), name_filter);
	if(name == 0xFFFF && szstart(pr->id, "Half"))
		name = speech_random_name(ids(pr->id + 4, pg->id), name_filter);
	return name;
}

void create_npc(npc* p, fnallowuc avatar_test, fnallowus name_filter) {
	p->alignment = last_alignment;
	p->race = last_race;
	p->gender = last_gender;
	p->character_class = last_class;
	p->name = generate_name(p->race, p->gender, name_filter);
	if(avatar_test)
		p->avatar = generate_avatar(p->race, p->gender, p->character_class, avatar_test);
	else
		p->avatar = 0xFF;
}

const char*	npc::getname() const {
	if(name == 0xFFFF)
		return "Noname";
	return speech_name(name);
}

void npc::say(const char* format, ...) const {
	XVA_FORMAT(format);
	sayv(format, format_param);
}

const char* npc_speech(const npc* player, const char* id, const char* action) {
	char temp[64]; stringbuilder sb(temp);
	auto& ei = player->getclass();
	for(auto i = 0; i < ei.count; i++) {
		sb.clear(); sb.add("%+1%+2%3", id, action, bsdata<classi>::elements[ei.classes[i]].id);
		auto p = speech_find(temp);
		if(p)
			return speech_get(p);
	}
	sb.clear(); sb.add("%+1%+2%3", id, action, bsdata<racei>::elements[player->race].id);
	auto p = speech_find(temp);
	if(p)
		return speech_get(p);
	sb.clear(); sb.add("%+1%+2", id, action);
	p = speech_find(temp);
	if(p)
		return speech_get(p);
	return 0;
}

void npc::speak(const char* id, const char* action, ...) const {
	auto format = npc_speech(this, id, action);
	if(!format)
		format = speech_get("WhatDoYouSay");
	XVA_FORMAT(action);
	sayv(format, format_param);
}

bool npc::speakn(const char* id, const char* action, ...) const {
	auto format = npc_speech(this, id, action);
	if(!format) {
		format = getnme(ids(id, action));
		if(format) {
			XVA_FORMAT(action);
			consolenl();
			consolev(format, format_param);
			return true;
		}
	}
	if(!format)
		return false;
	XVA_FORMAT(action);
	sayv(format, format_param);
	return true;
}

void npc::sayv(const char* format, const char* format_param) const {
	consolenl();
	console("%1 \"", getname());
	consolev(format, format_param);
	console("\"");
}

bool npc::isspecialist(const itemi* pi) const {
	auto i = bsdata<itemi>::source.indexof(pi);
	if(i == -1 || i >= 32)
		return false;
	return bsdata<racei>::elements[race].specialization.is(i);
}

int npc::getlevel(classn v) const {
	auto& e = bsdata<classi>::elements[character_class];
	if(e.classes[0] == v)
		return levels[0];
	else if(e.classes[1] == v)
		return levels[1];
	else if(e.classes[2] == v)
		return levels[2];
	return 0;
}

const racei& npc::getrace() const {
	return bsdata<racei>::elements[race];
}

const classi& npc::getclass() const {
	return bsdata<classi>::elements[character_class];
}

const classi& npc::getclassmain() const {
	return bsdata<classi>::elements[bsdata<classi>::elements[character_class].classes[0]];
}