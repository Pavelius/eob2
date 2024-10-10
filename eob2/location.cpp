#include "location.h"
#include "stringbuilder.h"

locationi* last_location;

const char*	locationi::getheader(const char* action) const {
	auto pn = getnme(ids(id, action));
	if(!pn)
		pn = getnm(id);
	return pn;
}