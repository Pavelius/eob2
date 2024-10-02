#pragma once

enum racen : unsigned char;
enum gendern : unsigned char;

void avatar_read(const char* url);
size_t get_avatars(unsigned char* result, racen race, gendern gender, char cls);
unsigned char get_avatar(racen race, gendern gender, char cls);
unsigned char get_avatar(racen race, gendern gender, char cls, unsigned char* exclude, size_t exclude_size);
