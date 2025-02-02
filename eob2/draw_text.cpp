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

namespace {
struct fxt {
	short int filesize; // the size of the file
	short int charoffset[128]; // the offset of the pixel data from the beginning of the file, the index is the ascii value
	unsigned char height; // the height of a character in pixel
	unsigned char width; // the width of a character in pixel
	unsigned char data[1]; // the pixel data, one byte per line 
};
}

int draw::texth() {
	if(!font)
		return 0;
	return ((fxt*)font)->height;
}

int draw::textw(int sym) {
	if(!font)
		return 0;
	return ((fxt*)font)->width;
}

void draw_glyph_zoomed(int sym, int zoom) {
	draw::rectpush push;
	auto f = (fxt*)draw::font;
	int height = f->height;
	int width = f->width;
	draw::width = zoom - 1;
	draw::height = zoom - 1;
	auto push_fore = draw::fore;
	auto back = draw::fore.mix(colors::black, 32);
	for(int h = 0; h < height; h++) {
		unsigned char line = *((unsigned char*)draw::font + ((fxt*)draw::font)->charoffset[sym] + h);
		unsigned char bit = 0x80;
		for(int w = 0; w < width; w++) {
			auto x0 = push.caret.x + w * zoom;
			auto y0 = push.caret.y + h * zoom;
			draw::caret.x = x0; draw::caret.y = y0;
			draw::fore = back; draw::rectf();
			draw::fore = push_fore;
			if((line & bit) == bit) {
				auto x1 = x0 + zoom;
				auto y1 = y0 + zoom;
				for(auto x = x0; x < x1; x++) {
					for(auto y = y0; y < y1; y++)
						draw::pixel(x, y);
				}
			}
			bit = bit >> 1;
		}
	}
	draw::fore = push_fore;
}

void draw::glyph(int sym, unsigned flags) {
	if(flags & TextBold) {
		auto push_caret = caret;
		auto push_fore = fore;
		fore = fore_stroke;
		caret.x -= 1;
		glyph(sym, 0);
		caret.y += 1;
		glyph(sym, 0);
		caret.x += 1;
		glyph(sym, 0);
		fore = push_fore;
		caret = push_caret;
	}
	auto f = (fxt*)font;
	int height = f->height;
	int width = f->width;
	for(int h = 0; h < height; h++) {
		unsigned char line = *((unsigned char*)font + ((fxt*)font)->charoffset[sym] + h);
		unsigned char bit = 0x80;
		for(int w = 0; w < width; w++) {
			if((line & bit) == bit)
				pixel(caret.x + w, caret.y + h);
			bit = bit >> 1;
		}
	}
}