#pragma once

// Use to clean up sequenced resources if destructor. Use it like 'clear(this)'.
template<class T>
inline void seqclear(T* p) {
	T* z = p->next;
	while(z) {
		T* n = z->next;
		z->next = 0;
		delete z;
		z = n;
	}
	p->next = 0;
}

// Return last element in sequence.
template<class T>
inline T* seqlast(T* p) {
	while(p->next)
		p = p->next;
	return p;
}

template<class T> inline void seqlink(T* p) {
	p->next = 0;
	if(!T::first)
		T::first = p;
	else
		seqlast(T::first)->next = p;
}
