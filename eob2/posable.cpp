#include "direction.h"
#include "posable.h"

posable last_exit;

pointc to(pointc v, directions d) {
	switch(d) {
	case Up: return {v.x, (char)(v.y - 1)};
	case Down: return {v.x, (char)(v.y + 1)};
	case Left: return {(char)(v.x - 1), v.y};
	case Right: return {(char)(v.x + 1), v.y};
	default: return v;
	}
}
