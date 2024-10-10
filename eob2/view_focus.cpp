#include "draw.h"
#include "math.h"
#include "slice.h"
#include "view_focus.h"

using namespace draw;

namespace {
struct renderi {
	void* av;
	point pt;
	void clear() { memset(this, 0, sizeof(*this)); }
};
}

static renderi render_objects[48];
static renderi*	render_current;

void *current_focus, *pressed_focus;
bool disable_input;

void focusing(const void* av) {
	if(!av || disable_input)
		return;
	if(!render_current
		|| render_current >= render_objects + sizeof(render_objects) / sizeof(render_objects[0]) - 1)
		render_current = render_objects;
	render_current[0].pt.x = caret.x + width / 2;
	render_current[0].pt.y = caret.y + height / 2;
	render_current[0].av = (void*)av;
	render_current++;
	render_current->clear();
	if(!current_focus)
		current_focus = (void*)av;
}

static renderi* getby(void* av) {
	for(auto& e : render_objects) {
		if(!e.av)
			return 0;
		if(e.av == av)
			return &e;
	}
	return 0;
}

static renderi* getfirst() {
	for(auto& e : render_objects) {
		if(!e.av)
			return 0;
		return &e;
	}
	return 0;
}

static renderi* getlast() {
	auto p = render_objects;
	for(auto& e : render_objects) {
		if(!e.av)
			break;
		p = &e;
	}
	return p;
}

//static bool is_horiz(point p1, point p2) {
//	return iabs(p1.x - p2.x) >= iabs(p1.y - p2.y);
//}

static bool is_vert(point p1, point p2) {
	return iabs(p1.x - p2.x) <= iabs(p1.y - p2.y);
}

static renderi* next_focus(void* ev, int key) {
	if(!key)
		return 0;
	auto pc = getby(ev);
	if(!pc)
		pc = getfirst();
	if(!pc)
		return 0;
	auto pe = pc;
	auto pl = getlast();
	int inc = 1;
	renderi* r1 = 0;
	auto p1 = pe->pt;
	while(true) {
		pc += inc;
		if(pc > pl)
			pc = render_objects;
		else if(pc < render_objects)
			pc = pl;
		if(pe == pc) {
			if(r1)
				return r1;
			return pe;
		}
		auto p2 = pc->pt;
		auto dx = iabs(p1.x - p2.x);
		auto dy = iabs(p1.y - p2.y);
		switch(key) {
		case KeyLeft:
			if(p2.x >= p1.x)
				continue;
			if(is_vert(p1, p2))
				continue;
			if(r1) {
				if(dy > iabs(p1.y - r1->pt.y))
					continue;
				if(distance(pe->pt, pc->pt) > distance(pe->pt, r1->pt))
					continue;
			}
			break;
		case KeyRight:
			if(p2.x <= p1.x)
				continue;
			if(is_vert(p1, p2))
				continue;
			if(r1) {
				if(dy > iabs(p1.y - r1->pt.y))
					continue;
				if(distance(pe->pt, pc->pt) > distance(pe->pt, r1->pt))
					continue;
			}
			break;
		case KeyDown:
			if(p2.y <= p1.y)
				continue;
			if(r1) {
				if(dx > iabs(p1.x - r1->pt.x))
					continue;
				if(distance(pe->pt, pc->pt) > distance(pe->pt, r1->pt))
					continue;
			}
			break;
		case KeyUp:
			if(p2.y >= p1.y)
				continue;
			if(r1) {
				if(dx > iabs(p1.x - r1->pt.x))
					continue;
				if(distance(pe->pt, pc->pt) > distance(pe->pt, r1->pt))
					continue;
			}
			break;
		default:
			return pc;
		}
		r1 = pc;
	}
}

void apply_focus(int key) {
	auto p = next_focus(current_focus, key);
	if(!p)
		return;
	current_focus = p->av;
}

void* focus_next(void* focus, int key) {
	auto p = next_focus(focus, key);
	if(!p)
		return 0;
	return p->av;
}

bool focus_input() {
	switch(hot.key) {
	case KeyLeft: apply_focus(KeyLeft); break;
	case KeyRight: apply_focus(KeyRight); break;
	case KeyUp: apply_focus(KeyUp); break;
	case KeyDown: apply_focus(KeyDown); break;
	default: return false;
	}
	return true;
}

void clear_focus_data() {
	render_current = 0;
}
