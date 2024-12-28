#include "draw.h"
#include "screenshoot.h"
#include "timer.h"

using namespace draw;

screenshoot::screenshoot(rect rc, bool fade) : surface(rc.width(), rc.height(), getbpp()) {
	x = rc.x1;
	y = rc.y1;
	if(draw::canvas)
		blit(*this, 0, 0, width, height, 0, *draw::canvas, x, y);
}

screenshoot::screenshoot(bool fade) : screenshoot({0, 0, getwidth(), getheight()}, fade) {
}

screenshoot::~screenshoot() {
}

void screenshoot::restore() const {
	setclip();
	if(draw::canvas)
		blit(*draw::canvas, x, y, width, height, 0, *this, 0, 0);
}

void screenshoot::blend(const screenshoot& destination, unsigned milliseconds) const {
	if(!milliseconds)
		return;
	auto start = getcputime();
	auto finish = start + milliseconds;
	auto current = start;
	while(ismodal() && current < finish) {
		auto alpha = ((current - start) << 8) / milliseconds;
		restore();
		canvas->blend(destination, alpha);
		doredraw();
		waitcputime(1);
		current = getcputime();
	}
}