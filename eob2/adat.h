#pragma once

#include "slice.h"

extern "C" int rand(void);

template<class T, size_t count_max = 128>
struct adat {
	typedef T data_type;
	size_t count;
	constexpr static size_t count_maximum = count_max;
	T data[count_max];
	adat() : count(0) {}
	constexpr const T& operator[](unsigned index) const { return data[index]; }
	constexpr T& operator[](unsigned index) { return data[index]; }
	explicit operator bool() const { return count != 0; }
	constexpr operator slice<T> () { return slice<T>(data, count);}
	T* add() { if(count < count_max) return data + (count++); return data; }
	void add(const T& e) { if(count < count_max) data[count++] = e; }
	void addu(const T& e) { if(!have(e)) add(e); }
	T* begin() { return data; }
	const T* begin() const { return data; }
	void clear() { count = 0; }
	T* end() { return data + count; }
	const T* end() const { return data + count; }
	const T* endof() const { return data + count_max; }
	int	find(const T t) const { for(auto& e : *this) if(e == t) return &e - data; return -1; }
	size_t getcount() const { return count; }
	size_t getmaximum() const { return count_max; }
	int	indexof(const void* e) const { if(e >= data && e < data + count) return (T*)e - data; return -1; }
	bool have(const T t) const { for(auto& e : *this) if(e == t) return true; return false; }
	bool have(const void* element) const { return element >= data && element < (data + count); }
	T random() const { return count ? data[rand() % count] : T(); }
	void random(size_t new_count) { shuffle(); new_count = (count < new_count) ? count : new_count; count = new_count; }
	void remove(int index, int remove_count = 1) { if(index < 0) return; if(index<int(count - 1)) memcpy(data + index, data + index + 1, sizeof(data[0]) * (count - index - 1)); count--; }
	void remove(const T t) { remove(find(t), 1); }
	void shuffle() { zshuffle(data, count); }
	void top(size_t v) { if(count > v) count = v; }
};
