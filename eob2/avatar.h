#pragma once

enum classn : unsigned char;
enum gendern : unsigned char;
enum racen : unsigned char;

typedef bool(*fnallowuc)(unsigned char);

void avatar_read(const char* url);
unsigned char generate_avatar(racen race, gendern gender, classn cls, fnallowuc filter);