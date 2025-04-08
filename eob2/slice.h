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

#ifndef _MSC_VER
using size_t = decltype(sizeof(0));
#endif // _MSC_VER

typedef bool(*fnallow)(const void* object, int index);
typedef void(*fncommand)(void* object);
typedef int (*fncompare)(const void*, const void*);
typedef void(*fnevent)(); // Callback function of any command executing
typedef bool(*fnvisible)(const void* object);
typedef const char*(*fngetname)(const void* object);

extern "C" void* bsearch(const void* key, const void* base, size_t num, size_t size, fncompare proc);
extern "C" void* memchr(const void* ptr, int value, long unsigned num);
extern "C" void* memcpy(void* destination, const void* source, long unsigned size) noexcept(true);
extern "C" int memcmp(const void* p1, const void* p2, size_t size) noexcept(true);
extern "C" void* memmove(void* destination, const void* source, size_t size) noexcept(true);
extern "C" void* memset(void* destination, int value, size_t size) noexcept(true);
extern "C" void	qsort(void* base, size_t num, size_t size, fncompare proc);

template<class T> class slice {
	T* data;
	size_t count;
public:
	typedef T data_type;
	constexpr slice() : data(0), count(0) {}
	template<size_t N> constexpr slice(T(&v)[N]) : data(v), count(N) {}
	constexpr T& operator[](size_t index) { return data[index]; }
	explicit operator bool() const { return count != 0; }
	constexpr const T& operator[](size_t index) const { return data[index]; }
	constexpr slice(T* data, unsigned count) : data(data), count(count) {}
	constexpr slice(T* p1, const T* p2) : data(p1), count(p2 - p1) {}
	constexpr T* begin() const { return data; }
	constexpr T* end() const { return data + count; }
	constexpr unsigned size() const { return count; }
};
