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

// Use to clean up sequenced resources if destructor. Use it like 'clear(this)'.
template<class T>
inline void seqclear(T* p) {
	T* z = p->next;
	while(z) {
		T* n = z->next;
		z->next = 0;
		delete z;
		z = n;
	}
	p->next = 0;
}

// Return last element in sequence.
template<class T>
inline T* seqlast(T* p) {
	while(p->next)
		p = p->next;
	return p;
}

template<class T> inline void seqlink(T* p) {
	p->next = 0;
	if(!T::first)
		T::first = p;
	else
		seqlast(T::first)->next = p;
}
