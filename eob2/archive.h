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

#include "adat.h"
#include "bsdata.h"
#include "io_stream.h"
#include "vector.h"

#pragma once

// Fast and simple driver for streaming binary data.
// Allow arrays and simple collections.
struct archive {
	io::stream&	source;
	bool writemode;
	constexpr archive(io::stream& source, bool writemode) : source(source), writemode(writemode) {}
	bool signature(const char* id);
	bool version(short major, short minor);
	bool checksum(unsigned long value);
	void set(void* value, unsigned size);
	void set(array& value);
	// Array with fixed count
	template<typename T, size_t N> void set(T(&value)[N]) {
		for(int i = 0; i < N; i++)
			set(value[i]);
	};
	// Fixed data collection
	template<typename T, unsigned N> void set(adat<T, N>& value) {
		set(value.count);
		for(auto& e : value)
			set(e);
	}
	// Fixed vector collection
	template<typename T> void set(vector<T>& value) {
		set(*static_cast<array*>(&value));
	}
	// All simple types and requisites
	template<class T> void set(T& value) {
		set(&value, sizeof(value));
	}
	void setname(const char*& value);
	void setpointer(void** value, array& source);
	void setpointerbyname(void** value, array& source);
	// Reference serialization (order in array is matters)
	template<class T> void set(T*& value) {
		setpointer((void**)&value, bsdata<T>::source);
		// Can be overloaded by setpointerbyname((void**)&value, bsdata<T>::source);
	}
};