#pragma once

extern void *current_focus, *pressed_focus;
extern bool disable_input;

struct pushfocus {
	void* focus;
	pushfocus() : focus(current_focus) { current_focus = 0; }
	~pushfocus() { current_focus = focus; }
};

void apply_focus(int key);
void clear_focus_data();
void focusing(const void* focus_data);
bool focus_input();

void* focus_next(void* focus, int key);