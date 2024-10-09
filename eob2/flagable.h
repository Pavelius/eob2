#pragma once

#define FG(V) (1<<V)

// Abstract flag set
template<unsigned N, typename T = unsigned char>
class flagable {
	static constexpr unsigned s = sizeof(T) * 8;
	T data[N];
public:
	constexpr explicit operator bool() const { for(auto e : data) if(e) return true; return false; }
	constexpr flagable() : data{0} {}
	constexpr bool is(short unsigned v) const { return (data[v / s] & (1 << (v % s))) != 0; }
	constexpr void remove(short unsigned v) { data[v / s] &= ~(1 << (v % s)); }
	constexpr void set(short unsigned v) { data[v / s] |= 1 << (v % s); }
};
// Abstract flag set partial
template<typename T>
class flagable<1, T> {
	static constexpr unsigned s = sizeof(T) * 8;
	T data;
public:
	constexpr explicit operator bool() const { return data != 0; }
	constexpr flagable() : data(0) {}
	constexpr bool is(short unsigned v) const { return (data & (1 << v)) != 0; }
	constexpr void remove(short unsigned v) { data &= ~(1 << v); }
	constexpr void set(short unsigned v) { data |= (1 << v); }
};
typedef flagable<1, unsigned long long> flag64;
typedef flagable<1, unsigned> flag32;
typedef flagable<1, unsigned short> flag16;
typedef flagable<1, unsigned char> flag8;