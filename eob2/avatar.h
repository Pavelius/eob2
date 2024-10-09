#pragma once

enum gendern : unsigned char;

void avatar_read(const char* url);
unsigned get_avatars(unsigned char* result, char race, gendern gender, char cls);
unsigned char generate_avatar(char race, gendern gender, char cls);
unsigned char generate_avatar(char race, gendern gender, char cls, unsigned char* exclude, unsigned long exclude_size);
