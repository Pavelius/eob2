#pragma once

#include "ability.h"
#include "feat.h"

struct statable : featable {
	char	abilities[Hits + 1];
	void	add(abilityn i, int v);
};
