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

static void serial_requisit(archive& f, void* object, const bsreq& type) {
	if(type.is(KindNumber) || type.is(KindEnum) || type.is(KindFlags) || type.is(KindDSet))
		f.set(object, type.lenght);
	else if(type.is(KindADat)) {
		auto p = (adat<int, 128>*)object;
		f.set(p->count);
		for(size_t i = 0; i < p->count; i++)
			f.set((char*)p->data + type.size * i, type.type);
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
			set(p->ptr(object), p->lenght);
		else {
			for(unsigned i = 0; i < p->count; i++)
				serial_requisit(*this, p->ptr(object, i), *p);
		}
	}
}

void archive::set(array& source, const bsreq* type) {
	if(!type)
		return;
	if(writemode)
		set(source.count);
	else {
		size_t size;
		set(size);
		source.reserve(size);
		source.setcount(size);
	}
	auto pe = source.end();
	for(auto p = source.begin(); p < pe; p += source.element_size)
		set((void*)p, type);
}