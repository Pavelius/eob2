#pragma once

#include "point.h"

typedef void(*fnevent)();
typedef void(*fnanswer)(int index, const void* value, const char* text, fnevent press_event);

enum resid : unsigned short;

void* choose_answer(point origin, resid background, int frame, int column_width);
void* choose_answer(const char* title, fnevent before_paint);

void button_label(int index, const void* data, const char* format, fnevent proc);
void initialize_gui();
void paint_adventure();
void paint_adventure_menu();
void text_label(int index, const void* data, const char* format, fnevent proc);
