#include "aflags.h"
#include "avatar.h"
#include "bsdata.h"
#include "class.h"
#include "log.h"
#include "gender.h"
#include "race.h"
#include "rand.h"
#include "stringbuilder.h"

using namespace log;

static struct avatari {
	gendern gender;
	aflags races;
	aflags classes;
} portrait_data[256];

static size_t get_avatars_ex(unsigned char* result, racen race, gendern gender, classn cls) {
	auto p = result;
	for(auto& e : portrait_data) {
		if(e.gender != NoGender && e.gender != gender)
			continue;
		if(!e.races.is(race))
			continue;
		if(!e.classes.is(cls))
			continue;
		*p++ = &e - portrait_data;
	}
	return p - result;
}

static size_t get_avatars_ex(unsigned char* result, racen race, gendern gender) {
	auto p = result;
	for(auto& e : portrait_data) {
		if(e.gender != NoGender && e.gender != gender)
			continue;
		if(!e.races.is(race))
			continue;
		*p++ = &e - portrait_data;
	}
	return p - result;
}

size_t get_avatars(unsigned char* result, racen race, gendern gender, classn cls) {
	auto c = get_avatars_ex(result, race, gender, cls);
	if(c < 4)
		c = get_avatars_ex(result, race, gender);
	return c;
}

unsigned char get_avatar(racen race, gendern gender, classn cls) {
	unsigned char result[256];
	auto c = get_avatars(result, race, gender, cls);
	if(!c)
		return 0;
	return result[rand() % c];
}

void avatar_read(const char* url) {
	auto p = log::read(url);
	if(!p)
		return;
	char temp[260]; stringbuilder sb(temp);
	auto line_number = 0;
	while(*p && allowparse) {
		if(*p == '\n' || *p=='\r') {
			p = skipcr(p);
			line_number++;
		} else {
			auto pe = portrait_data + line_number;
			sb.clear();
			p = sb.psidf(p);
			p = skipsp(p);
			auto p1 = bsdata<racei>::find(temp);
			if(p1) {
				pe->races.set((racen)(bsdata<racei>::source.indexof(p1)));
				continue;
			}
			auto p2 = bsdata<classi>::find(temp);
			if(p2) {
				pe->classes.set((classn)(bsdata<classi>::source.indexof(p2)));
				continue;
			}
			auto p3 = bsdata<genderi>::find(temp);
			if(p3) {
				pe->gender = (gendern)(bsdata<genderi>::source.indexof(p3));
				continue;
			}
			log::errorp(p, "Can't find gender, class or race named `%1`", temp);
		}
	}
	log::close();
}