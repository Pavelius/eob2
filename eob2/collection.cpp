#include "answers.h"
#include "collection.h"
#include "math.h"
#include "rand.h"
#include "stringbuilder.h"

static fngetname sort_proc;

static int compare_proc(const void* v1, const void* v2) {
	auto p1 = *((void**)v1);
	auto p2 = *((void**)v2);
	return szcmp(sort_proc(p1), sort_proc(p2));
}

static bool exist(void** pb, void** pe, const void* v) {
	while(pb < pe) {
		if(*pb == v)
			return true;
		pb++;
	}
	return false;
}

void collectiona::add(const collectiona& source) {
	for(auto p : source)
		add(p);
}

void collectiona::insert(int index, void* object) {
	if(count >= getmaximum())
		return;
	memmove(data + index + 1, data + index, (count - index - 1) * sizeof(data[0]));
	data[index] = object;
}

void collectiona::distinct() {
	auto ps = data;
	for(auto p : *this) {
		if(!exist(data, ps, p))
			*ps++ = p;
	}
	count = ps - data;
}

void collectiona::group(fngroup proc) {
	for(auto& e : *this)
		e = proc(e);
	distinct();
}

void collectiona::select(array& source) {
	auto ps = data;
	auto pe = endof();
	auto ae = source.end();
	auto s = source.size();
	for(auto p = source.begin(); p < ae; p += s) {
		if(ps >= pe)
			break;
		*ps++ = p;
	}
	count = ps - data;
}

void collectiona::select(array& source, fnvisible proc, bool keep) {
	if(!proc) {
		select(source);
		return;
	}
	auto ps = data;
	auto pe = endof();
	auto ae = source.end();
	auto s = source.size();
	for(auto p = source.begin(); p < ae; p += s) {
		if(ps >= pe)
			break;
		if(proc(p) != keep)
			continue;
		*ps++ = p;
	}
	count = ps - data;
}

void collectiona::select(array& source, fnallow proc, int param, bool keep) {
	if(!proc) {
		select(source);
		return;
	}
	auto ps = data;
	auto pe = endof();
	auto ae = source.end();
	auto s = source.size();
	for(auto p = source.begin(); p < ae; p += s) {
		if(ps >= pe)
			break;
		if(proc(p, param) != keep)
			continue;
		*ps++ = p;
	}
	count = ps - data;
}

void collectiona::match(fnvisible proc, bool keep) {
	auto ps = data;
	for(auto p : *this) {
		if(proc(p) != keep)
			continue;
		*ps++ = p;
	}
	count = ps - data;
}

void collectiona::match(const collectiona& source, bool keep) {
	auto ps = data;
	for(auto p : *this) {
		if((source.find(p) != -1) != keep)
			continue;
		*ps++ = p;
	}
	count = ps - data;
}

void collectiona::match(fnallow proc, int param, bool keep) {
	auto ps = data;
	for(auto p : *this) {
		if(proc(p, param) != keep)
			continue;
		*ps++ = p;
	}
	count = ps - data;
}

void* collectiona::random() const {
	if(!count)
		return 0;
	return data[rand() % count];
}

void collectiona::sort(fngetname proc) {
	auto push_proc = sort_proc; sort_proc = proc;
	qsort(data, count, sizeof(data), compare_proc);
	sort_proc = push_proc;
}

void collectiona::sort(fncompare proc) {
	qsort(data, count, sizeof(data[0]), proc);
}

void collectiona::shuffle() {
	zshuffle(data, count);
}

void* collectiona::pick() {
	auto result = (count > 0) ? data[0] : 0;
	remove(0, 1);
	return result;
}

void collectiona::top(int number) {
	if(count > (size_t)number)
		count = number;
}