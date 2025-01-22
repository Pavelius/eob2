#pragma once

struct historyi {
	short unsigned p1, p2;
	char stage, value;
};

historyi* find_history(short unsigned p1, short unsigned p2);
historyi* add_history(short unsigned p1, short unsigned p2);
int get_history(short unsigned p1, short unsigned p2);