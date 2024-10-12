#include "bsdata.h"
#include "direction.h"
#include "shape.h"
#include "stringbuilder.h"
#include "math.h"
#include "log.h"

using namespace log;

BSDATAC(shapei, 128)

static const char* shape_symbols = {"UX.1234567890 "};

pointc shapei::find(char sym) const {
	pointc v;
	for(v.y = 0; v.y < size.y; v.y++) {
		for(v.x = 0; v.x < size.x; v.x++) {
			if((*this)[v] == sym)
				return v;
		}
	}
	return {-1, -1};
}

pointc shapei::translate(pointc c, pointc v, directions d) const {
	switch(d) {
	case Up: return c.to(origin.x + v.x, origin.y + v.y);
	case Down: return c.to(origin.x + v.x, -origin.y - v.y);
	case Left: return c.to(v.y + origin.y, v.x);
	case Right: return c.to(size.y - v.y + origin.y, origin.x + v.x);
		//case Left: return c.to(-origin.y - v.y, v.x);
		//case Right: return c.to(v.y, -origin.x - v.x);
	default: return c;
	}
}

static bool isallowed(char sym) {
	return zchr(shape_symbols, sym) != 0;
}

static const char* read_block(const char* p, shapei& e, stringbuilder& sb) {
	while(*p == '\n' || *p == '\r')
		p = skipcr(p);
	sb.clear();
	auto ps = sb.get();
	auto pb = p;
	e.size.x = 0;
	e.size.y = 0;
	e.origin.x = 0;
	e.origin.y = 0;
	if(!isallowed(*p))
		errorp(p, "Expected shape data");
	while(allowparse && *p && (isallowed(*p) || *p == '\n' || *p == '\r')) {
		if((*p == '\n') || (*p == '\r')) {
			if(e.size.x == 0)
				e.size.x = p - pb;
			else {
				auto n = p - pb;
				if(n != e.size.x)
					errorp(p, "Shape row '%2' must be %1i characters", e.size.x, ps);
			}
			while(*p == '\n' || *p == '\r')
				p = skipcr(p);
			pb = p;
			ps = sb.get();
			e.size.y++;
		} else
			sb.add(*p++);
	}
	if(e.size.x)
		e.size.y = sb.size() / e.size.x;
	size_t mr = e.size.x * e.size.y;
	size_t mn = sb.getmaximum();
	if(mr > mn)
		errorp(pb, "Shape size %1ix%2i is too big. Try make it smallest. Multiplied width and height of shape must be not greater that %3i.", e.size.x, e.size.y, mn);
	e.content = szdup(sb.begin());
	for(auto sym : "0123456789")
		e.points[sym - '0'] = e.find(sym);
	e.origin.x = -e.points[0].x;
	e.origin.y = -e.points[0].y;
	return skipspcr(p);
}

void shape_read(const char* url) {
	auto p = log::read(url);
	if(!p)
		return;
	char temp[8192]; stringbuilder sb(temp);
	allowparse = true;
	while(allowparse && *p) {
		if(!checksym(p, '#'))
			break;
		auto ps = bsdata<shapei>::add();
		p = psidf(p + 1, sb);
		ps->id = szdup(temp);
		if(!checksym(p, '\n'))
			break;
		p = read_block(p, *ps, sb);
	}
	log::close();
}