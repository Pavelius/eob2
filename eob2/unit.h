#pragma once

struct creaturei;

struct uniti {
	creaturei*	units[6];
	void		clear();
	bool		is(const creaturei* v) const;
};