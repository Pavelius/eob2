#include "location.h"
#include "stringbuilder.h"

locationi* last_location;

const char*	locationi::getheader(const char* action) const {
	auto pn = getnme(ids(id, action));
	if(!pn && group)
		pn = getnme(ids(group, action));
	if(!pn)
		pn = getnm(id);
	return pn;
}

const char*	locationi::getname() const {
	auto pn = getnme(id);
	if(!pn && group)
		pn = getnme(group);
	if(!pn)
		pn = getnm(id);
	return pn;
}