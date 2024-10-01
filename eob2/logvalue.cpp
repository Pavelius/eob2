#include "logvalue.h"

const char* log::valuei::read(const char* p, stringbuilder& sb) {
	clear();
	if(*p == '\"') {
		sb.clear(); p = sb.psstr(p + 1, *p);
		text = szdup(sb.begin());
	} else if(*p == '-' || isnum(*p)) {
		auto minus = false;
		if(*p == '-') {
			minus = true;
			p++;
		}
		p = psnum(p, number);
		if(minus)
			number = -number;
	} else if(ischa(p[0])) {
		sb.clear(); p = sb.psidf(p);
		identifier = (const char*)sb.begin();
		if(identifier) {
			int bonus; p = psbon(p, bonus);
			identifier.counter = bonus;
			data = identifier.getpointer();
		} else
			text = szdup(sb.begin());
	}
	return p;
}

const char* log::psval(const char* p, variant& v) {
	if(!ischa(p[0]))
		return p;
	char temp[128]; stringbuilder sb(temp);
	sb.clear(); p = sb.psidf(p);
	v = (const char*)sb.begin();
	int bonus; p = psbon(p, bonus);
	v.counter = bonus;
	return p;
}