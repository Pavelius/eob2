#pragma once

enum gendern : unsigned char;

struct npc {
	char			race, type;
	gendern			gender;
	unsigned short	name;
	const char*		getname() const;
	void			say(const char* format, ...) const;
	void			sayv(const char* format, const char* format_param) const;
};
unsigned short generate_name(int race, gendern gender);