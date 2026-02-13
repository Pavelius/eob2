#include "avatar.h"
#include "bsreq.h"
#include "draw.h"
#include "log.h"
#include "midi.h"
#include "music.h"
#include "party.h"
#include "quest.h"
#include "rand.h"
#include "speech.h"
#include "timer.h"
#include "textscript.h"
#include "script.h"
#include "view.h"

void util_main();

static void check_locale_exist(const char* id, const char* suffix) {
	char temp[128]; stringbuilder sb(temp);
	sb.add("%1%2", id, suffix);
	auto pn = getnme(temp);
	if(pn)
		return;
	log::errorp(0, "Not define locale string `%1`", temp);
}

static void check_quest_locals() {
	char temp[128]; stringbuilder sb(temp); sb.add("locale/%1/Campaign.txt", current_locale);
	auto push = log::context;
	log::context.seturl(temp);
	for(auto& e : bsdata<quest>()) {
		check_locale_exist(e.id, "Summary");
		check_locale_exist(e.id, "Agree");
		check_locale_exist(e.id, "Finish");
	}
}

int main() {
	start_random_seed = getcputime();
	// start_random_seed = 1423089921;
	// current_locale[0] = 'u';
	// current_locale[1] = 'a';
	srand(start_random_seed);
	initialize_gui();
	music_initialize();
	bsreq::read("rules/Core.txt");
	avatar_read("rules/Avatars.txt");
	// check_localizations("en", "ua");
	// check_speech("en", "ua");
	initialize_translation();
	initialize_strings();
	log::readlf(speech_read, "locale", "*.str");
	check_quest_locals();
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
	song_play("kvirasim");
	set_next_scene(main_menu);
	run_next_scene();
}

#ifdef _MSC_VER
int _stdcall WinMain(void* ci, void* pi, char* cmd, int sw) {
	return main();
}
#endif