#include "bsreq.h"
#include "groupname.h"
#include "log.h"
#include "rand.h"
#include "stringbuilder.h"

using namespace log;

BSDATAC(groupname, 2048);
BSMETA(groupname) = {
	BSREQ(parent),
	BSREQ(name),
	{}};

size_t select_group_name(unsigned short* result, size_t count, const char* parent) {
	auto pb = result;
	auto pe = result + count;
	for(auto& e : bsdata<groupname>()) {
		if(szpmatch(e.parent, parent)) {
			if(pb < pe)
				*pb++ = &e - bsdata<groupname>::elements;
		}
	}
	return pb - result;
}

const groupname* random_group_name(const char* parent) {
	unsigned short result[512];
	auto count = select_group_name(result, lenghtof(result), parent);
	if(!count)
		return 0;
	return bsdata<groupname>::elements + rand() % count;
}

const char* get_group_name(short unsigned v) {
	if(v==0xFFFF)
		return "";
	return bsdata<groupname>::elements[v].name;
}

short unsigned random_group_namei(const char* parent) {
	auto pn = random_group_name(parent);
	if(!pn)
		return 0xFFFF;
	return pn - bsdata<groupname>::elements;
}

static const char* read_line(const char* p, const char* parent, stringbuilder& sb) {
	while(ischa(*p)) {
		auto pe = bsdata<groupname>::add();
		memset(pe, 0, sizeof(*pe));
		sb.clear();
		pe->parent = parent;
		p = sb.psparam(skipws(p));
		pe->name = szdup(sb);
		p = skipws(p);
		if(*p == 13 || *p == 10 || *p == 0)
			break;
		if(!checksym(p, ','))
			break;
		p = skipwscr(p + 1);
	}
	return p;
}

void read_group_names(const char* url) {
	auto p = log::read(url);
	if(!p)
		return;
	char temp[4096]; stringbuilder sb(temp);
	allowparse = true;
	while(allowparse && *p) {
		if(!checksym(p, '#'))
			break;
		sb.clear();
		p = sb.psidf(skipws(p + 1));
		if(!checksym(p, '\n'))
			break;
		p = read_line(skipwscr(p), szdup(temp), sb);
		p = skipwscr(p);
	}
	close();
}