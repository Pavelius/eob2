#pragma once

#include "nameable.h"

typedef int(*fnformula)(int op1, int op2);

struct formulai : nameable {
	fnformula	proc;
};
extern int last_number;
