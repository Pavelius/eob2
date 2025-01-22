#pragma once

#include "color.h"

enum resid : unsigned short;

struct picturei {
	resid	res;
	short	frame;
	explicit operator bool() const { return res != (resid)0; }
	void	clear() { res = (resid)0; frame = 0; }
};
extern picturei picture;
