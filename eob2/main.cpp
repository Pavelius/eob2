#include "answers.h"
#include "creature.h"
#include "draw.h"
#include "log.h"
#include "rand.h"
#include "resid.h"
#include "timer.h"
#include "widget.h"

extern "C" void exit(int code);

static void exit_game() {
	exit(0);
}

static void start_game() {
}

static void main_menu() {
	auto push = an;
	an.clear();
	an.add(start_game, "Begin new game");
	an.add(exit_game, "Load saved game");
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