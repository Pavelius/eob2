#pragma once

#include "bsdata.h"

template<typename T>
struct sliceu {
	unsigned start, count;
	constexpr sliceu() : start(0), count(0) {}
	constexpr sliceu(unsigned start, unsigned count) : start(start), count(count) {}
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
