#pragma once

enum classn : unsigned char;

struct levelable {
	classn character_class;
	char levels[3];
	int	experience;
	int getlevel() const { return levels[0]; }
	int getlevel(classn v) const;
};