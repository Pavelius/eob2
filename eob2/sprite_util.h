#include "draw.h"

#pragma once

void pma_write(const char* url, pma** pp);
void* sprite_add(sprite* p, const void* data, int dsize);
void sprite_create(sprite* p, int count, int cicles = 0, int additional_bytes = 0);
int sprite_store(sprite* ps, const unsigned char* p, int width, int w, int h, int ox, int oy, sprite::encodes mode = sprite::Auto, unsigned char shadow_index = 0, color* original_pallette = 0, int explicit_frame = -1, unsigned char transparent_index = 0);
void sprite_write(const char* url, const sprite* p);