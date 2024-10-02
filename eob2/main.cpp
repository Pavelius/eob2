#include "answers.h"
#include "avatar.h"
#include "bsreq.h"
#include "creature.h"
#include "draw.h"
#include "gender.h"
#include "groupname.h"
#include "log.h"
#include "rand.h"
#include "resid.h"
#include "timer.h"
#include "unit.h"
#include "view.h"

extern "C" void exit(int code);

static void exit_game() {
	exit(0);
}

static void start_game() {
}

static void main_menu() {
	pushanswer push;
	create_player(bsdata<racei>::find("Human"), Male, bsdata<classi>::find("Fighter"));
	join_party();
	create_player(bsdata<racei>::find("Elf"), Male, bsdata<classi>::find("Fighter"));
	join_party();
	create_player(bsdata<racei>::find("Human"), Female, bsdata<classi>::find("Fighter"));
	join_party();
	create_player(bsdata<racei>::find("Dwarf"), Male, bsdata<classi>::find("Cleric"));
	join_party();
	create_player(bsdata<racei>::find("Halfling"), Female, bsdata<classi>::find("Theif"));
	join_party();
	an.clear();
	an.add(start_game, "Begin new game");
	an.add(exit_game, "Load saved game");
	auto p = choose_answer("Game options:", paint_adventure_menu, button_label, 2);
	//auto p = choose_answer(0, paint_main_menu, text_label, 1);
}

int main() {
	srand(getcputime());
	initialize_gui();
	initialize_translation();
	log::readlf(read_group_names, "names", "*.txt");
	bsreq::read("rules/Core.txt");
	avatar_read("rules/Avatars.txt");
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