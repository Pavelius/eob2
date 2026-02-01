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

unsigned char* get_font_data(int sym) {
	return (unsigned char*)draw::font + ((fxt*)draw::font)->charoffset[sym];
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