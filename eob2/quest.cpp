#include "quest.h"
#include "party.h"

quest* last_quest;
quest* party_quests[128];

void clear_quests() {
	memset(party_quests, 0, sizeof(party_quests));
}

bool last_quest_complite() {
	if(!last_quest)
		return false;
	auto quest_id = getbsi(last_quest);
	for(auto i = (goaln)0; i <= KillAlmostAllMonsters; i = (goaln)(i + 1)) {
		if(!last_quest->goals[i])
			continue;
		auto value = party_goal(quest_id, i);
		if(value < last_quest->goals[i])
			return false;
	}
	return true;
}