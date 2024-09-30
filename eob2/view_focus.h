#pragma once

extern void *current_focus, *pressed_focus;

void focusing(const void* focus_data);
void focus_input();

void* focus_next(void* focus, int key);