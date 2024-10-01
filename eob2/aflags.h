#pragma once

class aflags {
	unsigned data;
public:
	constexpr aflags() : data(0) {}
	constexpr explicit operator bool() const { return data != 0; }
	constexpr void add(const aflags& e) { data |= e.data; }
	constexpr bool allof(const aflags& e) const { return (data & e.data) == data; }
	constexpr void clear() { data = 0; }
	constexpr bool is(const int v) const { return (data & (1 << v)) != 0; }
	constexpr bool is(const aflags& e) const { return (data & e.data) != 0; }
	constexpr void remove(const int v) { data &= ~(1 << v); }
	constexpr void set(const int v) { data |= (1 << v); }
};
