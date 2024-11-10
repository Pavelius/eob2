#pragma once

struct referencei {
	short unsigned parent, index;
	constexpr explicit operator bool() const { return parent != 0xFFFF && index != 0xFFFF; }
	template<typename T> operator T*() const; // Externally defined function
	constexpr bool operator==(const referencei& v) const { return parent == v.parent && index == v.index; }
	constexpr bool operator!=(const referencei& v) const { return parent != v.parent || index != v.index; }
	constexpr referencei() : parent(0xFFFF), index(0xFFFF) {}
	template<typename T> referencei(T* v); // Externally defined function
	template<typename T> referencei(const T* v) : referencei(const_cast<T*>(v)) {}
	void clear() { parent = 0xFFFF; index = 0xFFFF; }
};
