#pragma once

bool midi_busy();
bool midi_play(const char* file_name);
void midi_play_raw(void* mid_data);
void midi_sleep(unsigned milliseconds);
bool midi_music_played();
bool midi_music_played(const void* mid_data);
void midi_music_stop();