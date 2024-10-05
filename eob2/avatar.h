#pragma once

enum gendern : unsigned char;

void avatar_read(const char* url);
size_t get_avatars(unsigned char* result, char race, gendern gender, char cls);
unsigned char generate_avatar(char race, gendern gender, char cls);
unsigned char generate_avatar(char race, gendern gender, char cls, unsigned char* exclude, size_t exclude_size);
