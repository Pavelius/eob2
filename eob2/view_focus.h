#pragma once

extern void *current_focus, *pressed_focus;
extern bool disable_input;

struct pushfocus {
	void* focus;
	pushfocus() : focus(current_focus) {}
	~pushfocus() { current_focus = focus; }
};

void clear_focus_data();
void focusing(const void* focus_data);
void focus_input();

void* focus_next(void* focus, int key);