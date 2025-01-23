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

#define maprnd(t) t[rand()%(sizeof(t)/sizeof(t[0]))]

extern "C" int rand(void);
extern "C" void srand(unsigned seed); // Set random seed

inline int d20() { return 1 + rand() % 20; }
inline int d100() { return rand() % 100; }
inline int xrand(int n1, int n2) { return n1 + rand() % (n2 - n1 + 1); }

template<class T>
inline void zshuffle(T* p, int count) {
	for(int i = 0; i < count; i++) {
		auto j = rand() % count;
		auto v = p[i];
		p[i] = p[j];
		p[j] = v;
	}
}