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

#include "variant.h"

#pragma once

enum modifiern : unsigned char;

struct script {
	typedef void(*fnrun)(int bonus);
	const char*	id;
	fnrun		proc;
};
struct modifieri {
	const char*	id;
};
struct scriptbody : variants {
	scriptbody();
};
extern modifiern modifier;
extern variant* script_begin;
extern variant* script_end;
extern const char* last_id;

bool script_allow(variant v);
bool script_allow(const variants& elements, bool apply_and = true);
void script_stop();
bool script_stopped();
void script_run();
void script_run(variant v);
void script_run(const variants& elements);
void script_run(const char* id, const variants& elements);

variant next_script();

template<typename T> bool fttest(int index, int value);
template<typename T> void ftscript(int index, int value);