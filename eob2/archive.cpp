#include "archive.h"
#include "stringbuilder.h"
#include "variant.h"

bool archive::signature(const char* id) {
	char temp[4];
	if(writemode) {
		memset(temp, 0, sizeof(temp));
		stringbuilder sb(temp);
		sb.addv(id, 0);
		set(temp, sizeof(temp));
	} else {
		set(temp, sizeof(temp));
		if(szcmpi(temp, id) != 0)
			return false;
	}
	return true;
}

bool archive::checksum(unsigned long value) {
	if(writemode)
		set(value);
	else {
		decltype(value) new_value;
		set(new_value);
		if(new_value != value)
			return false;
	}
	return true;
}

bool archive::version(short major, short minor) {
	short major_reader = major;
	short minor_reader = minor;
	set(major_reader);
	set(minor_reader);
	if(!writemode) {
		if(major_reader < major)
			return false;
		else if(major_reader == major && minor_reader < minor)
			return false;
	}
	return true;
}

void archive::set(array& v) {
	set(v.count);
	set(v.element_size);
	if(!writemode)
		v.reserve(v.count);
	set(v.data, v.element_size * v.count);
}

void archive::set(void* value, unsigned size) {
	if(writemode)
		source.write(value, size);
	else
		source.read(value, size);
}

void archive::setpointer(void** value) {
	if(writemode) {
		variant v;
		auto pi = find_variant(*value);
		if(pi) {
			v.type = pi - bsdata<varianti>::elements;
			v.value = pi->source->indexof(*value);
			v.counter = 0;
		} else
			v.clear();
		set(v);
	} else {
		variant v; set(v);
		auto pi = bsdata<varianti>::elements[v.type].source;
		if(!pi)
			*value = 0;
		else
			*value = pi->ptr(v.value);
	}
}