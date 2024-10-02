#include "variant.h"

#pragma once

enum modifiern : unsigned char;

struct script {
	typedef void(*fnrun)(int bonus);
	typedef bool(*fntest)(int bonus);
	const char*		id;
	fnrun			proc;
	fntest			test;
};
struct modifieri {
	const char*	id;
};
extern modifiern modifier;
extern variant* script_begin;
extern variant* script_end;
extern bool script_fail;

int script_count(int count, int minimal = 1);

bool script_allow(variant v);
bool script_allow(const variants& elements);
void script_execute(const char* id, int bonus = 0);
void script_stop();
bool script_stopped();
void script_run(variant v);
void script_run(variant v, int counter);
void script_run(const variants& elements);

variant next_script();

template<typename T> bool fttest(int index, int value);
template<typename T> void ftscript(int index, int value);