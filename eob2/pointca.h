#pragma once

#include "posable.h"
#include "adat.h"

extern unsigned short pathmap[mpy][mpx];

struct pointca : adat<pointc, 1024> {
	void	select(int r1, int r2);
};