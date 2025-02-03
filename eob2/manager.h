#pragma once

#include "array.h"
#include "nameable.h"

struct manageri : nameable {
	void*	data;
	int		size;
	bool	error;
	void	clear();
};

void manager_clear(array& source);
void manager_initialize(array& source, const char* folder, const char* filter);

void* manager_get(array& source, const char* id, const char* folder, const char* ext);

