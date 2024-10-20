#include "bsreq.h"
#include "log.h"
#include "nameable.h"
#include "rand.h"
#include "speech.h"
#include "stringbuilder.h"

using namespace log;

BSDATAD(speech::element)
BSDATAC(speech, 1024)

BSMETA(speech) = {
	BSREQ(id),
	{}};

unsigned char* speech_params;

void speech_read(const char* url) {
	auto p = log::read(url);
	if(!p)
		return;
	int mode = 0;
	char temp[4096]; stringbuilder sb(temp);
	allowparse = true;
	while(allowparse && *p) {
		if(!checksym(p, '#'))
			break;
		sb.clear();
		p = sb.psidf(p + 1);
		if(equal(temp, "SetMode")) {
			p = skipws(p);
			p = psnum(p, mode);
			p = skipwscr(skipline(p));
			continue;
		}
		auto pr = bsdata<speech>::add();
		pr->id = szdup(temp);
		if(!checksym(p, '\n'))
			break;
		p = skipwscr(p);
		auto psb = bsdata<speech::element>::source.count;
		switch(mode) {
		case 0:
			while(allowparse && *p && *p != '#') {
				sb.clear();
				p = sb.psstrlf(skipwscr(p));
				p = skipwscr(p);
				speech::element e = {};
				e.name = szdup(temp);
				bsdata<speech::element>::source.add(&e);
			}
			break;
		case 1:
			while(allowparse && ischa(*p)) {
				speech::element e = {};
				sb.clear();
				p = sb.psparam(skipws(p));
				e.name = szdup(sb);
				bsdata<speech::element>::source.add(&e);
				p = skipws(p);
				if(*p == 13 || *p == 10 || *p == 0)
					break;
				if(!checksym(p, ','))
					break;
				p = skipwscr(p + 1);
			}
			p = skipwscr(p);
			break;
		}
		if(psb != bsdata<speech::element>::source.count)
			pr->source.set((speech::element*)bsdata<speech::element>::source.ptr(psb), bsdata<speech::element>::source.count - psb);
	}
	log::close();
}

const char* speech_getid(int index) {
	return bsdata<speech>::elements[index].id;
}

const char* speech_name(int index) {
	return ((speech::element*)bsdata<speech::element>::source.ptr(index))->name;
}

const speech* speech_find(const char* id) {
	return bsdata<speech>::find(id);
}

int speech_first(const speech* p) {
	if(!p || !p->source)
		return -1;
	return p->source.start;
}

int speech_count(const speech* p) {
	if(!p || !p->source)
		return -1;
	return p->source.count;
}

int speech_random(const char* id) {
	auto p = bsdata<speech>::find(id);
	if(!p || !p->source)
		return -1;
	return p->source.start + (rand() % p->source.size());
}

short unsigned speech_random_name(const char* pattern, fnallowus name_filter) {
	unsigned short result[512];
	auto count = select_speech(result, lenghtof(result), pattern);
	if(!count)
		return 0xFFFF;
	if(name_filter)
		count = filter_speech(result, count, name_filter, false);
	if(!count)
		return 0xFFFF;
	return result[rand() % count];
}

const char* speech_get(const speech* p) {
	if(!p || !p->source)
		return 0;
	auto n = (speech_params ? *speech_params++ : rand()) % p->source.size();
	return p->source.begin()[n].name;
}

const char* speech_get(const char* id) {
	return speech_get(bsdata<speech>::find(id));
}

const char* speech_get(const speech* p, int n) {
	if(!p || !p->source)
		return 0;
	return p->source.begin()[n].name;
}

const char* speech_get(const char* id, const char* action) {
	auto pn = ids(id, action);
	auto p = speech_get(pn);
	if(p)
		return p;
	p = getnme(pn);
	if(p)
		return p;
	return speech_get(ids("Global", action));
}

void speech_get(const char*& result, const char* id, const char* action, const char* postfix) {
	if(result)
		return;
	char temp[64]; stringbuilder sb(temp);
	sb.add(id);
	sb.add(action);
	sb.add(postfix);
	auto p = speech_get(temp);
	if(p)
		result = p;
}

bool apply_speech(const char* id, stringbuilder& sb) {
	auto p = speech_get(id);
	if(!p)
		return false;
	sb.add(p);
	return true;
}

bool parse_speech(stringbuilder& sb, const char* id) {
	for(auto& e : bsdata<speech>()) {
		if(!equal(e.id, id))
			continue;
		if(!e.source.size())
			continue;
		auto n = (speech_params ? *speech_params++ : rand()) % e.source.size();
		auto pn = e.source.begin()[n].name;
		sb.add(pn);
		return true;
	}
	return false;
}

size_t select_speech(unsigned short* result, size_t count, const char* parent) {
	auto pb = result;
	auto pe = result + count;
	for(auto& e : bsdata<speech>()) {
		if(szpmatch(e.id, parent)) {
			for(auto& ei : e.source) {
				if(pb < pe)
					*pb++ = bsdata<speech::element>::source.indexof(&ei);
			}
		}
	}
	return pb - result;
}

size_t filter_speech(unsigned short* result, size_t count, fnallowus allow_proc, bool keep) {
	auto ps = result;
	auto pe = result + count;
	for(auto pb = result; pb < pe; pb++) {
		if(allow_proc(*pb) != keep)
			continue;
		*ps++ = *pb;
	}
	return ps - result;
}