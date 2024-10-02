#pragma once

struct groupname {
	const char*	parent;
	const char*	name;
};

size_t select_group_name(unsigned short* result, size_t count, const char* parent);
const groupname* random_group_name(const char* parent);
unsigned short random_group_namei(const char* parent);
const char* get_group_name(short unsigned v);

void read_group_names(const char* url);

