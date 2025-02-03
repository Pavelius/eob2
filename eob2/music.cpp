#include "bsdata.h"
#include "midi.h"
#include "music.h"
#include "thread.h"

static volatile void* played_music;

io::thread music_tread;

void music_clear() {
	manager_clear(bsdata<musici>::source);
}

void music_initialize() {
	manager_initialize(bsdata<musici>::source, "music", "*.mid");
}

void* song_get(const char* id) {
	return manager_get(bsdata<musici>::source, id, "music", "mid");
}

static void music_play_background(void* music_data) {
	midi_play_raw(music_data);
	played_music = 0;
}

void music_play(void* new_music) {
	if(played_music == new_music)
		return;
	if(played_music)
		midi_music_stop();
	while(played_music && midi_busy())
		midi_sleep(10);
	played_music = new_music;
	if(new_music) {
		music_tread.close();
		music_tread.start(music_play_background, new_music);
	}
}

void song_play(const char* id) {
	music_play(song_get(id));
}