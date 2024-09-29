#pragma once

template<typename T>
class pushvalue {
	T&	ref;
public:
	T	value;
	pushvalue(T& ref) : ref(ref), value(ref) {}
	pushvalue(T& ref, T nv) : ref(ref), value(ref) { ref = nv; }
	~pushvalue() { ref = value; }
};
