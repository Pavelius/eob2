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

#include "adat.h"

template<typename T>
struct bsindex {
	static adat<T*> source;
	static short unsigned add(T* v) { if(!v) return 0xFFFF; auto i = find(v); if(i != 0xFFFF) return i; auto p1 = source.add(); *p1 = p; return p1 - source.data; }
	static short unsigned find(T* v) { return source.find(v); }
	static constexpr T* begin() { return (T*)source.data; }
	static constexpr T* end() { return (T*)source.data + source.getcount(); }
};
