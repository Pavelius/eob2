#pragma once

enum racen : unsigned char;
enum gendern : unsigned char;
enum classn : unsigned char;

size_t get_avatars(unsigned char* result, racen race, gendern gender, classn cls);
unsigned char get_avatar(racen race, gendern gender, classn cls);
