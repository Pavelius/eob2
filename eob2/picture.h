#pragma once

enum resid : unsigned short;

struct picturei {
	resid	id;
	short	frame;
	explicit operator bool() const { return id != (resid)0; }
};
extern picturei picture;