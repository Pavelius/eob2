#pragma once

enum alignmentn : unsigned char;
enum classn : unsigned char;
enum gendern : unsigned char;
enum racen : unsigned char;

struct itemi;

typedef bool(*fnallowuc)(unsigned char);
typedef bool(*fnallowus)(unsigned short);

struct npc {
	alignmentn		alignment;
	racen			race;
	gendern			gender;
	classn			type;
	unsigned char	avatar;
	unsigned short	name;
	const char*		getname() const;
	bool			isspecialist(const itemi* pi) const;
	void			say(const char* format, ...) const;
	void			sayv(const char* format, const char* format_param) const;
	void			speak(const char* id, const char* action, ...) const;
	bool			speakn(const char* id, const char* action, ...) const;
};

void create_npc(npc* p, fnallowuc avatar_test, fnallowus name_test);