#pragma once

enum resid : unsigned short;

struct picturei {
	resid	res;
	short	frame;
	explicit operator bool() const { return res != (resid)0; }
};
extern picturei picture;
