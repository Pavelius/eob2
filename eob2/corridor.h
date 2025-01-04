#pragma once

#include "direction.h"
#include "pointc.h"
#include "nameable.h"

typedef void (*fncorridor)(pointc v, directions d);

struct corridori : nameable {
	fncorridor	proc;
};