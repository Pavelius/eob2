#include "avatar.h"
#include "alignment.h"
#include "bsdata.h"
#include "console.h"
#include "class.h"
#include "gender.h"
#include "speech.h"
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
	p->type = last_class;
	p->name = generate_name(p->race, p->gender, name_filter);
	p->avatar = generate_avatar(p->race, p->gender, p->type, avatar_test);
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
	auto& ei = bsdata<classi>::elements[player->type];
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