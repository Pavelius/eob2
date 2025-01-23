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

#include "bsdata.h"
#include "bsreq.h"
#include "formula.h"
#include "script.h"

int last_number;

static int add_formula(int p1, int p2) {
	return p1 + p2;
}

static int div_formula(int p1, int p2) {
	return p1 / p2;
}

static int sub_formula(int p1, int p2) {
	return p1 - p2;
}

static int mul_formula(int p1, int p2) {
	return p1 * p2;
}

static int set_formula(int p1, int p2) {
	return p2;
}

template<> void ftscript<formulai>(int value, int bonus) {
	last_number = bsdata<formulai>::elements[value].proc(last_number, bonus);
}

BSMETA(formulai) = {
	BSREQ(id),
	{}};
BSDATA(formulai) = {
	{"Add", add_formula},
	{"Div", div_formula},
	{"Mul", mul_formula},
	{"Set", set_formula},
	{"Sub", sub_formula},
};
BSDATAF(formulai)