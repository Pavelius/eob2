#pragma once

#include "point.h"

const int walls_frames = 9;
const int walls_count = 6;
const int door_offset = 1 + walls_frames * walls_count;
const int decor_offset = door_offset + 9;
const int decor_count = 21;
const int decor_frames = 10;
const int scrx = 22 * 8;
const int scry = 15 * 8;

typedef void(*fnevent)();
typedef void(*fnanswer)(int index, const void* value, const char* text, unsigned key, fnevent press_event);

enum resid : unsigned short;
struct hotkeyi;

extern unsigned long current_cpu_time;

void* choose_answer(const char* title, const char* cancel, fnevent before_paint, fnanswer answer_paint, int padding = 2);
void* choose_dialog(const char* title, int dx);
void* show_message(const char* format, bool add_anaswers, const char* cancel = 0, unsigned cancel_key = 27);
void* dialogv(const char* cancel, const char* format);
void* dialogv(const char* cancel, const char* format, const char* format_param);
void* dialog(const char* cancel, const char* format, ...);
void choose_spells(const char* title, const char* cancel, int spell_type);
void show_scene(fnevent before_paint, fnevent input, void* focus);

bool adventure_input(const hotkeyi* hotkeys);
void alternate_focus_input();
void animation_update();
void button_label(int index, const void* data, const char* format, unsigned key, fnevent proc);
bool character_input();
void city_input(const hotkeyi* hotkeys);
void clear_input();
bool confirm(const char* format);
bool hotkey_input(const hotkeyi* hotkeys);
void initialize_gui();
void paint_adventure();
void paint_avatars();
void paint_avatars_no_focus();
void paint_avatars_no_focus_hilite();
void paint_city();
void paint_city_menu();
void paint_dungeon();
void paint_main_menu();
void paint_party_status();
void set_dungeon_tiles(resid type);
void set_player_by_focus();
void set_small_font();
void text_label(int index, const void* data, const char* format, unsigned key, fnevent proc);
