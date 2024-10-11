#include "adat.h"
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
	void set(void* value, unsigned size);
	void setpointer(void** value);
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
	template<class T> void set(T*& value) { setpointer((void**)&value); }
};