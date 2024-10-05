#pragma once

#include "point.h"

typedef void(*fnevent)();
typedef void(*fnanswer)(int index, const void* value, const char* text, fnevent press_event);

enum resid : unsigned short;

void* choose_answer(const char* title, fnevent before_paint, fnanswer answer_paint, int padding = 2);

void button_label(int index, const void* data, const char* format, fnevent proc);
void city_scene(fnevent before_paint);
void initialize_gui();
void paint_adventure();
void paint_adventure_menu();
void paint_city();
void paint_main_menu();
void text_label(int index, const void* data, const char* format, fnevent proc);
