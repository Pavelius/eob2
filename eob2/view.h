#pragma once

#include "point.h"

typedef void(*fnevent)();
typedef void(*fnanswer)(int index, const void* value, const char* text, unsigned key, fnevent press_event);

enum resid : unsigned short;

void* choose_answer(const char* title, const char* cancel, fnevent before_paint, fnanswer answer_paint, int padding = 2);
void* choose_dialog(const char* title, int dx);

void alternate_focus_input();
void button_label(int index, const void* data, const char* format, unsigned key, fnevent proc);
bool character_input();
void city_input(fnevent menu_proc);
void clear_input();
void show_scene(fnevent before_paint, fnevent input);
void initialize_gui();
void paint_adventure();
void paint_avatars();
void paint_avatars_no_focus();
void paint_city();
void paint_city_menu();
void paint_main_menu();
void paint_party_status();
void text_label(int index, const void* data, const char* format, fnevent proc);
