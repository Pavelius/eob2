#pragma once

bool midi_busy();
void midi_music_stop();
void midi_open();
void midi_play_raw(void* mid_data);
void midi_repeat(bool value);
void midi_sleep(unsigned milliseconds);