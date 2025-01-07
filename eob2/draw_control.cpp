#include "draw.h"
#include "draw_control.h"

using namespace draw;

const controli* last_control;

void paint_controls(const controli* controls, size_t count) {
	rectpush push;
	auto push_last = last_control;
	auto pe = last_control + count;
	for(last_control = controls; last_control < pe; last_control++) {
		caret.x = last_control->x;
		caret.y = last_control->y;
		width = last_control->width;
		height = last_control->height;
		last_control->proc();
	}
}