#include "avatar.h"
#include "bsreq.h"
#include "draw.h"
#include "log.h"
#include "party.h"
#include "rand.h"
#include "speech.h"
#include "timer.h"
#include "textscript.h"
#include "script.h"
#include "view.h"

void util_main();

int main() {
	//start_random_seed = getcputime();
	start_random_seed = 2831817;
	srand(start_random_seed);
	initialize_gui();
	initialize_translation();
	initialize_strings();
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
//	if(!read_game("autosave")) {
		script_run("StartGame");
		//save_game("autosave");
//	}
	run_next_scene();
}

#ifdef _MSC_VER
int _stdcall WinMain(void* ci, void* pi, char* cmd, int sw) {
	return main();
}
#endif
