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

#pragma once

#include "slice.h"

struct array {
	void* data;
	size_t count, element_size, count_maximum;
	constexpr array(size_t size = 0) : data(0), count(0), element_size(size), count_maximum(0) {}
	constexpr array(void* data, size_t size, size_t count) : data(data), count(count), element_size(size), count_maximum(count | 0x80000000) {}
	constexpr array(void* data, size_t size, size_t count, unsigned count_maximum) : data(data), count(count), element_size(size), count_maximum(count_maximum | 0x80000000) {}
	constexpr explicit operator bool() const { return count != 0; }
	~array();
	void* add();
	void* addz() { auto p = add(); memset(p, 0, element_size); return p; }
	void* add(const void* element);
	void* addfind(const char* id);
	void* addu(const void* element, unsigned count);
	const char* addus(const char* element, unsigned count);
	char* begin() const { return (char*)data; }
	void change(unsigned offset, int size);
	void clear();
	char* end() const { return (char*)data + element_size * count; }
	int find(int i1, int i2, void* value, unsigned offset, size_t size) const;
	int find(void* value, unsigned offset, size_t size) const { return find(0, -1, value, offset, size); }
	const void* findu(const void* value, size_t size) const;
	const char* findus(const char* value, size_t size) const;
	void* findv(const char* value, unsigned offset) const;
	void* findv(const char* value, unsigned offset, size_t size) const;
	size_t getmaximum() const { return count_maximum & 0x7FFFFFFF; }
	size_t getcount() const { return count; }
	size_t size() const { return element_size; }
	constexpr bool have(const void* element) const { return element >= data && element < (char*)data + element_size * count; }
	int indexof(const void* element) const;
	void* insert(int index, const void* element);
	bool isgrowable() const { return (count_maximum & 0x80000000) == 0; }
	void* ptr(int index) const { return (char*)data + element_size * index; }
	void* ptrs(int index) const { return (((unsigned)index) < count) ? (char*)data + element_size * index : 0; }
	template<class T> slice<T> records() const { return slice<T>((T*)data, count); }
	void remove(int index, int elements_count = 1);
	void remove();
	void shift(int i1, int i2, size_t c1, size_t c2);
	void setcount(unsigned value) { count = value; }
	void setup(size_t size);
	void swap(int i1, int i2);
	void reserve(unsigned count);
	void repack(unsigned& start, unsigned count);
private:
	void grow(unsigned offset, size_t delta);
	void shrink(unsigned offset, size_t delta);
	void zero(unsigned offset, size_t delta);
};
