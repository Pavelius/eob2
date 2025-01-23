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
#include "bsdata.h"
#include "io_stream.h"
#include "vector.h"

#define BSRAW(R) {FO(data_type,R), sizeof(data_type::R)}
#define BSARH(T) template<> const archive::record archive::type<T>::meta[]

// Fast and simple driver for streaming binary data.
// Allow arrays and simple collections.
struct archive {
	struct record {
		size_t offset;
		size_t size;
	};
	template<class T> struct type {
		typedef T data_type;
		static const record meta[];
	};
	io::stream&	source;
	bool writemode;
	constexpr archive(io::stream& source, bool writemode) : source(source), writemode(writemode) {}
	bool signature(const char* id);
	bool version(short major, short minor);
	bool checksum(unsigned long value);
	void set(void* value, unsigned size);
	void set(array& value);
	void set(array& value, const record* metadata); // Serial array, but when read - search by name and not create new.
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
	template<class T> void setbinary() { set(bsdata<T>::source, type<T>::meta); }
};