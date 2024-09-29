#include "ability.h"

void add_value(char& result, int i, int minimum, int maximum) {
	i += result;
	if(i < minimum)
		i = minimum;
	else if(i > maximum)
		i = maximum;
	result = i;
}