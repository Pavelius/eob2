#include "answers.h"
#include "rand.h"

bool show_interactive = true;
answers an;

int answers::compare(const void* v1, const void* v2) {
	return szcmp(((answers::element*)v1)->text, ((answers::element*)v2)->text);
}

void answers::addv(const void* value, const char* text, const char* format, unsigned key) {
	auto p = elements.add();
	p->value = value;
	p->text = sc.get();
	p->key = key;
	sc.addv(text, format);
	sc.addsz();
}

void answers::sort() {
	qsort(elements.data, elements.count, sizeof(elements.data[0]), compare);
}

void* answers::random() const {
	if(!elements.count)
		return 0;
	return (void*)elements.data[rand() % elements.count].value;
}

const char* answers::getname(void* v) {
	for(auto& e : elements) {
		if(e.value == v)
			return e.text;
	}
	return 0;
}

void answers::clear() {
	elements.clear();
	sc.clear();
}

size_t answers::findvalue(const void* pv) const {
	for(auto& e : elements) {
		if(e.value == pv)
			return &e - elements.data;
	}
	return -1;
}