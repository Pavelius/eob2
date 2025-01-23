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

#pragma once

#include "sliceu.h"

typedef bool(*fnallowus)(unsigned short);

struct speech {
	struct element {
		const char*	name;
	};
	typedef sliceu<element> elementa;
	const char*	id;
	elementa	source;
};

void speech_read(const char* url);
void speech_get(const char*& result, const char* id, const char* action, const char* postfix = 0);

const speech* speech_find(const char* id);

int speech_count(const speech* p);
int speech_first(const speech* p);
int speech_random(const char* id);
short unsigned speech_random_name(const char* pattern, fnallowus name_filter);

const char* speech_name(int index);
const char* speech_getid(int index);
const char* speech_get(const speech* p);
const char* speech_get(const char* id);
const char* speech_get(const char* id, const char* action);
const char* speech_get_na(const char* id, const char* action);
const char* speech_get(const speech* p, int n);

size_t select_speech(unsigned short* result, size_t count, const char* parent);
size_t filter_speech(unsigned short* result, size_t count, fnallowus filter_proc, bool keep);

extern unsigned char* speech_params;