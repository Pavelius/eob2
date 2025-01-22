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
