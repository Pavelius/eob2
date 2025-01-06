#include "hashsum.h"
#include "nameable.h"

unsigned long hashsum(const char* format) {
	if(!format)
		return 0;
	unsigned long total = 0;
	for(auto i = 0; format[i]; i++)
		total += ((unsigned char)format[i]) * (i + 1);
	return total;
}

unsigned long hashsum(array& source) {
	unsigned long total = 0;
	int index = 1;
	auto pe = source.end();
	auto size = source.size();
	for(auto p = source.begin(); p < pe; p += size)
		total += hashsum(((nameable*)p)->id) * (index++);
	return total;
}