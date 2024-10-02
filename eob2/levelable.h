#pragma once

struct levelable {
	char character_class;
	char levels[3];
	int	experience;
	int getclassindex(int id) const;
	int getlevel() const { return levels[0]; }
	int getlevel(int v) const;
};