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

#include "bsdata.h"

template<typename T>
struct sliceu {
	unsigned start, count;
	constexpr sliceu() : start(0), count(0) {}
	constexpr sliceu(unsigned start, unsigned count) : start(start), count(count) {}
	constexpr sliceu(T& value) { set(&value, 1); }
	template<size_t N> sliceu(T(&v)[N]) { set(v, N); }
	constexpr explicit operator bool() const { return count != 0; }
	constexpr operator slice<T>() { return slice<T>(begin(), count); }
	T*		begin() const { return (T*)bsdata<T>::source.ptr(start); }
	void	clear() { start = count = 0; }
	T*		end() const { return (T*)bsdata<T>::source.ptr(start + count); }
	void	repack() { bsdata<T>::source.repack(start, count); }
	void	set(const T* v, unsigned count) { start = bsdata<T>::source.indexof(v); this->count = count; }
	void	setu(const T* v, unsigned count) { start = bsdata<T>::source.indexof(bsdata<T>::source.addu(v, count)); this->count = count; }
	void	setbegin() { start = bsdata<T>::source.getcount(); count = 0; }
	void	setend() { count = bsdata<T>::source.getcount() - start; }
	constexpr unsigned size() const { return count; }
};
