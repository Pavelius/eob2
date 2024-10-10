#pragma once

#include "nameable.h"

typedef int(*fnformula)(int op1, int op2);

struct formulai : nameable {
	fnformula	proc;
};
extern int last_number;

int add_formula(int p1, int p2);
int mul_formula(int p1, int p2);
int set_formula(int p1, int p2);
int sub_formula(int p1, int p2);
