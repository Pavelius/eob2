#include "nameable.h"
#include "stringbuilder.h"

const char*	nameable::getname() const {
	return getnm(id);
}