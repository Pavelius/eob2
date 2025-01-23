/////////////////////////////////////////////////////////////////////////
// 
// Copyright 2024 Pavel Chistyakov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http ://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "archive.h"
#include "nameable.h"
#include "stringbuilder.h"

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
	else if(value)
		source.read(value, size);
	else // Skip same count of bytes if readed object is empthy
		source.seek(size, SeekCur);
}

void archive::setname(const char*& value) {
	short unsigned v;
	if(writemode) {
		if(value) {
			v = (short unsigned)zlen(value);
			set(v);
			set((void*)value, v);
		} else {
			v = 0;
			set(v);
		}
	} else {
		char temp[1024];
		char* p = temp;
		set(v);
		if(v > sizeof(temp) - 2)
			p = new char[v];
		set(p, v);
		p[v] = 0;
		value = szdup(p);
		if(p != temp)
			delete[] p;
	}
}

void archive::setpointer(void** value, array& source) {
	unsigned short v;
	if(writemode) {
		v = source.indexof(*value);
		set(v);
	} else {
		set(v);
		*value = (v==0xFFFF) ? 0 : source.ptr(v);
	}
}

void archive::setpointerbyname(void** value, array& source) {
	const char* p;
	if(writemode) {
		p = 0;
		if(*value)
			p = ((nameable*)(*value))->id;
		setname(p);
	} else {
		setname(p);
		if(value)
			*value = source.findv(p, 0);
	}
}