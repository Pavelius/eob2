#include "avatar.h"
#include "bsreq.h"
#include "draw.h"
#include "speech.h"
#include "log.h"
#include "rand.h"
#include "timer.h"
#include "script.h"
#include "view.h"

void util_main();

int main() {
	srand(getcputime());
	initialize_gui();
	initialize_translation();
	log::readlf(speech_read, "names", "*.txt");
	bsreq::read("rules/Core.txt");
	avatar_read("rules/Avatars.txt");
	if(log::errors > 0)
		return -1;
#ifdef _DEBUG
	util_main();
#endif
	if(log::errors > 0)
		return -1;
	draw::create(-1, -1, 320, 200, 0, 32);
	draw::setcaption("Eye of beholder (remake)");
	draw::settimer(100);
	script_run("StartGame");
}

int _stdcall WinMain(void* ci, void* pi, char* cmd, int sw) {
	return main();
}