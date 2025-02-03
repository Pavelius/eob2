#pragma once

#include "manager.h"

struct musici : manageri {
};

void music_clear();
void music_initialize();
void music_play(void* mid_raw_data);
void music_play(void* mid_raw_data);
void song_play(const char* id);

void* song_get(const char* id);
