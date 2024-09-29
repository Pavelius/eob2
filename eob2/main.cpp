#include "answers.h"
#include "creature.h"
#include "draw.h"
#include "log.h"
#include "rand.h"
#include "resid.h"
#include "timer.h"
#include "widget.h"

static void main_menu() {
	auto push = an;
	an.clear();
	an.add(0, "Begin new game");
	an.add(0, "Load saved game");
	auto p = choose_answer({80, 110}, MENU, 0, 166);
}

int main() {
	srand(getcputime());
	initialize_gui();
	initialize_translation();
	if(log::errors > 0)
		return -1;
#ifdef _DEBUG
#endif
	if(log::errors > 0)
		return -1;
	draw::create(-1, -1, 320, 200, 0, 32);
	draw::setcaption("Eye of beholder (remake)");
	draw::settimer(100);
	main_menu();
}

int _stdcall WinMain(void* ci, void* pi, char* cmd, int sw) {
	return main();
}