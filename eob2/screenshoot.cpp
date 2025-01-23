/////////////////////////////////////////////////////////////////////////
// 
// Copyright 2024 Pavel Chistyakov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http ://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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