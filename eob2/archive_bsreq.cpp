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
#include "bsreq.h"

static char* sptr(void* object, const bsreq* type, int i) {
	if(!object)
		return 0;
	return type->ptr(object, i);
}

static char* sptr(void* object, const bsreq* type) {
	if(!object)
		return 0;
	return type->ptr(object);
}

static void serial_requisit(archive& f, void* object, const bsreq& type) {
	if(type.is(KindNumber) || type.is(KindEnum) || type.is(KindFlags) || type.is(KindDSet))
		f.set(object, type.lenght);
	else if(type.is(KindADat)) {
		if(object) {
			auto p = (adat<int, 128>*)object;
			f.set(p->count);
			for(size_t i = 0; i < p->count; i++)
				f.set((char*)p->data + type.size * i, type.type);
		} else if(!f.writemode) {
			decltype(adat<int, 128>::count) count;
			f.set(count);
			f.source.seek(type.size * count, SeekCur);
		}
	} else if(type.is(KindScalar))
		f.set(object, type.type);
	else if(type.is(KindText))
		f.setname(*(const char**)object);
	else if(type.is(KindReference)) {
		if(type.type->is(KindText)) // First requisit must be `id` or other text value
			f.setpointerbyname((void**)object, *type.source);
	} else if(type.is(KindSlice))
		return; // Not support
}

void archive::set(void* object, const bsreq* type) {
	if(!type)
		return; // Error unknown type
	for(auto p = type; *p; p++) {
		if(p->is(KindNumber) || p->is(KindEnum) || p->is(KindFlags) || p->is(KindDSet)) // Optimization
			set(sptr(object, p), p->lenght);
		else {
			for(unsigned i = 0; i < p->count; i++)
				serial_requisit(*this, sptr(object, p, i), *p);
		}
	}
}

void archive::set(array& source, const bsreq* type) {
	if(!type)
		return;
	if(writemode)
		set(source.count);
	else {
		decltype(source.count) size; set(size);
		source.reserve(size);
		source.setcount(size);
	}
	auto pe = source.end();
	for(auto p = source.begin(); p < pe; p += source.element_size)
		set((void*)p, type);
}

void archive::setng(array& source, const bsreq* type) {
	if(!type)
		return;
	if(!type->is(KindText))
		return; // Not `nameable` element. Not support this.
	if(writemode)
		set(source, type); // Same as standart case
	else {
		decltype(source.count) size; set(size);
		for(decltype(size) i = 0; i < size; i++) {
			const char* name;
			setname(name);
			auto object = source.findv(name, 0);
			set((void*)object, type + 1);
		}
	}
}