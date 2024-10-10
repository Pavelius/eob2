#include "variant.h"

#pragma once

namespace log {
struct valuei {
	const char* text;
	long		number;
	void*		data;
	variant	identifier;
	void clear() { memset(this, 0, sizeof(*this)); }
	const char* read(const char* p, stringbuilder& sb);
};
const char*		psval(const char* p, variant& v);
}
