#pragma once

#include "nameable.h"
#include "sliceu.h"
#include "stringbuilder.h"

#define VAR(T, K) bsmeta<T>::meta, bsdata<T>::source_ptr, K

struct bsreq;
union variant;

typedef void (*fngetinfo)(const void* object, variant v, stringbuilder& sb);

struct varianti : nameable {
	typedef void(*fnscript)(int index, int bonus);
	typedef void(*fnread)(const char* url);
	typedef bool(*fntest)(int index, int bonus);
	const bsreq*	metadata;
	array*			source;
	int             key_count;
	fnstatus		pstatus;
	fngetinfo		pgetinfo;
	fnscript		pscript;
	fntest			ptest;
	fnread			pread;
	fnevent			pinitialize;
	static const varianti* getsource(const char* id);
	const char*		getid(const void* object) const;
	const char*		getname(const void* object) const;
	int				found(const char* id, size_t size) const;
	constexpr bool	isnamed() const { return key_count==1; }
};
union variant {
	unsigned char	uc[4];
	unsigned		u;
	struct {
		unsigned short value;
		char		counter;
		unsigned char type;
	};
	constexpr variant() : u(0) {}
	constexpr variant(unsigned char type, char counter, unsigned short value) : value(value), counter(counter), type(type) {}
	template<class T> variant(T* v) : variant((const void*)v) {}
	constexpr explicit operator bool() const { return u != 0; }
	constexpr bool operator==(const variant& v) const { return u == v.u; }
	constexpr bool operator!=(const variant& v) const { return u != v.u; }
	template<class T> operator T*() const { return (T*)((bsdata<varianti>::elements[type].source == bsdata<T>::source_ptr) ? getpointer() : 0); }
	template<class T> operator const T*() const { return (T*)((bsdata<varianti>::elements[type].source == bsdata<T>::source_ptr) ? getpointer() : 0); }
	void			clear() { u = 0; }
	constexpr bool	issame(const variant& v) const { return type == v.type && value == v.value; }
	constexpr variant nocounter() const { return variant(type, 0, value); }
	template<class T> constexpr bool iskind() const { return bsdata<varianti>::elements[type].source==bsdata<T>::source_ptr; }
	const varianti&	to() const { return bsdata<varianti>::elements[type]; }
	const char*		getid() const;
	void*			getpointer() const { return to().source->ptr(value); }
	const char*		getname() const;
};
template<> variant::variant(const char* v);
template<> variant::variant(const void* v);

typedef sliceu<variant> variants;
typedef void (*fnvariant)(variant v);
typedef bool (*fntestvariant)(variant v);

const varianti* find_variant(const void* object);

unsigned long bsreq_signature();
unsigned long bsreq_name_count_signature();
