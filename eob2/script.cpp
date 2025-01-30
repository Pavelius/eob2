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

#include "bsreq.h"
#include "rand.h"
#include "script.h"

BSMETA(script) = {
	BSREQ(id),
	{}};
BSMETA(modifieri) = {
	BSREQ(id),
	{}};

variant* script_begin;
variant* script_end;
modifiern modifier;
const char* last_id;

template<> void ftscript<modifieri>(int value, int counter) {
	modifier = (modifiern)value;
}
template<> bool fttest<modifieri>(int value, int counter) {
	ftscript<modifieri>(value, counter);
	return true;
}
template<> void ftscript<script>(int value, int counter) {
	bsdata<script>::elements[value].proc(counter);
}

scriptbody::scriptbody() : variants(bsdata<variant>::source.indexof(script_begin), script_end - script_begin) {
}

void script_stop() {
	script_begin = script_end;
}

bool script_stopped() {
	return script_begin == script_end;
}

bool script_allow(variant v) {
	auto proc = bsdata<varianti>::elements[v.type].ptest;
	if(proc)
		return proc(v.value, v.counter);
	return true;
}

bool script_allow(const variants& elements, bool apply_and) {
	auto push_begin = script_begin;
	auto push_end = script_end;
	script_begin = elements.begin();
	script_end = elements.end();
	auto result = true;
	if(apply_and) {
		while(script_begin < script_end) {
			if(!script_allow(*script_begin++)) {
				result = false;
				break;
			}
		}
	} else {
		result = false;
		while(script_begin < script_end) {
			if(script_allow(*script_begin++)) {
				result = true;
				break;
			}
		}
	}
	script_begin = push_begin;
	script_end = push_end;
	return result;
}

void script_run() {
	while(script_begin < script_end)
		script_run(*script_begin++);
}

void script_run(variant v) {
	auto proc = bsdata<varianti>::elements[v.type].pscript;
	if(proc)
		proc(v.value, v.counter);
}

void script_run(const variants& elements) {
	auto push_begin = script_begin;
	auto push_end = script_end;
	script_begin = elements.begin();
	script_end = elements.end();
	while(script_begin < script_end)
		script_run(*script_begin++);
	script_begin = push_begin;
	script_end = push_end;
}

void script_run(const char* id, const variants& elements) {
	auto push_id = last_id; last_id = id;
	auto push_modifier = modifier;
	script_run(elements);
	modifier = push_modifier;
	last_id = push_id;
}

variant next_script() {
	if(script_begin && script_begin < script_end)
		return *script_begin++;
	return variant();
}