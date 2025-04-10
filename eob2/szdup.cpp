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

#include "stringbuilder.h"
#include "slice.h"

template<class T> static void remove(T* p) {
	T* z = p->next;
	while(z) {
		T* n = z->next;
		z->next = 0;
		delete z;
		z = n;
	}
	p->next = 0;
}

// Support class making string copy from strings storage.
struct strcol {

	strcol*	next;
	int		count;
	char	data[256 * 255]; // Inner buffer

	strcol() : next(0), count(0), data() {
	}

	~strcol() {
		remove(this);
	}

	bool has(const char* value) {
		strcol* e = this;
		while(e) {
			if(value >= e->data && value <= e->data + e->count)
				return true;
			e = e->next;
		}
		return false;
	}

	const char* find(const char* text, int textc) {
		if(textc == -1)
			textc = zlen(text);
		const char c = text[0];
		for(strcol* t = this; t; t = t->next) {
			int m = t->count - textc;
			if(m < 0)
				continue;
			for(int i = 0; i < m; i++) {
				if(c == data[i]) {
					int	j = 1;
					auto p = &data[i];
					for(; j < textc; j++)
						if(p[j] != text[j])
							break;
					if(j == textc && p[j] == 0)
						return p;
				}
			}
		}
		return 0;
	}

	const char* add(const char* text, int textc) {
		if(!text)
			return 0;
		if(has(text))
			return text;
		if(textc == -1)
			textc = zlen(text);
		auto r = find(text, textc);
		if(r)
			return r;
		strcol* t = this;
		while(true) {
			if((unsigned)(t->count + textc + 1) > sizeof(data) / sizeof(data[0])) {
				if(!t->next)
					t->next = new strcol;
				if(!t->next)
					return 0;
				t = t->next;
				continue;
			}
			auto result = &t->data[t->count];
			memcpy(result, text, textc * sizeof(text[0]));
			result[textc] = 0;
			t->count += textc + 1;
			return result;
		}
	}

};

const char* szdup(const char* text) {
	static strcol small;
	static strcol big;
	if(!text)
		return 0;
	if(text[0] == 0)
		return "";
	//text = zskipspcr(text);
	int lenght = zlen(text);
	if(lenght < 32)
		return small.add(text, lenght);
	else
		return big.add(text, lenght);
}

const char* szdupz(const char* text) {
	if(!text || text[0] == 0)
		return 0;
	return szdup(text);
}