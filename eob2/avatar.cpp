#include "flagable.h"
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
	flagable<1, unsigned> races;
	flagable<1, unsigned> classes;
} portrait_data[256];

static bool contain(unsigned char* result, size_t result_size, unsigned char value) {
	auto pe = result + result_size;
	for(auto p = result; p < pe; p++) {
		if(*p == value)
			return true;
	}
	return false;
}

static size_t filter(unsigned char* result, size_t result_size, unsigned char* source, size_t source_size) {
	if(!source_size)
		return result_size;
	auto pe = result + result_size;
	auto ps = result;
	for(auto p = result; p < pe; p++) {
		if(contain(source, source_size, *p))
			continue;
		*ps++ = *p;
	}
	return ps - result;
}

static size_t get_avatars_ex(unsigned char* result, char race, gendern gender, char cls) {
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

static size_t get_avatars_ex(unsigned char* result, char race, gendern gender) {
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

size_t get_avatars(unsigned char* result, char race, gendern gender, char cls, unsigned char* exclude, size_t exclude_size) {
	auto c = get_avatars_ex(result, race, gender, cls);
	c = filter(result, c, exclude, exclude_size);
	if(c < 4) {
		c = get_avatars_ex(result, race, gender);
		c = filter(result, c, exclude, exclude_size);
	}
	return c;
}

unsigned char generate_avatar(char race, gendern gender, char cls) {
	unsigned char result[256];
	auto c = get_avatars(result, race, gender, cls, 0, 0);
	if(!c)
		return 0;
	return result[rand() % c];
}

unsigned char generate_avatar(char race, gendern gender, char cls, unsigned char* exclude, size_t exclude_size) {
	unsigned char result[256];
	auto c = get_avatars(result, race, gender, cls, exclude, exclude_size);
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
				pe->races.set(bsdata<racei>::source.indexof(p1));
				continue;
			}
			auto p2 = bsdata<classi>::find(temp);
			if(p2) {
				pe->classes.set(bsdata<classi>::source.indexof(p2));
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