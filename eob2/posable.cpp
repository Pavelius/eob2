#include "direction.h"
#include "posable.h"

pointc to(pointc v, directions d) {
	switch(d) {
	case Up: return {v.x, v.y - (char)1};
	case Down: return {v.x, v.y + (char)1};
	case Left: return {v.x - 1, v.y};
	case Right: return {v.x + 1, v.y};
	default: return v;
	}
}