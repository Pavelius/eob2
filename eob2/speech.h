#pragma once

#include "sliceu.h"

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
short unsigned speech_random_name(const char* pattern);

const char* speech_name(int index);
const char* speech_getid(int index);
const char* speech_get(const char* id);
const char* speech_get(const speech* p, int n);

size_t select_speech(unsigned short* result, size_t count, const char* parent);

extern unsigned char* speech_params;