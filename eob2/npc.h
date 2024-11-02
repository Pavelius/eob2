#pragma once

enum alignmentn : unsigned char;
enum classn : unsigned char;
enum gendern : unsigned char;
enum racen : unsigned char;

struct classi;
struct itemi;
struct racei;

typedef bool(*fnallowuc)(unsigned char);
typedef bool(*fnallowus)(unsigned short);

struct npc {
	alignmentn		alignment;
	racen			race;
	gendern			gender;
	classn			character_class;
	unsigned char	avatar;
	unsigned short	name;
	char			levels[3];
	int				experience;
	const classi&	getclass() const;
	const classi&	getclassmain() const;
	int				getlevel(classn v) const;
	int				getlevel() const { return levels[0]; }
	const char*		getname() const;
	const racei&	getrace() const;
	bool			isspecialist(const itemi* pi) const;
	void			say(const char* format, ...) const;
	void			sayv(const char* format, const char* format_param) const;
	void			speak(const char* id, const char* action, ...) const;
	bool			speakn(const char* id, const char* action, ...) const;
};

void create_npc(npc* p, fnallowuc avatar_test, fnallowus name_test);