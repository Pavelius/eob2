#include "adat.h"
#include "carousing.h"
#include "creature.h"
#include "party.h"
#include "script.h"
#include "view.h"

typedef adat<carousingi*, 256> carousinga;
carousingi* last_carousing;

static void create_source(carousinga& source) {
	source.clear();
	for(auto& e : bsdata<carousingi>())
		source.add(&e);
	source.shuffle();
}

static carousingi* pick(carousinga& source) {
	for(auto& p : source) {
		if(p->filter) {

		}
		auto result = p;
		auto index = source.indexof(&p);
		if(index!=-1)
			source.remove(index, 1);
		return result;
	}
	return 0;
}

static void apply_carousing() {
	auto push = last_id;
	auto push_avatar = picture;
	picture = last_carousing->avatar;
	last_id = last_carousing->id;
	dialog(getnm("Continue"), getnm(last_id));
	party.abilities[EffectCount] = 0;
	if(player->roll(last_carousing->ability)) {
		script_run(last_carousing->fail);
		dialog(getnm("Continue"), getnm(ids(last_id, "Fail")));
	} else {
		script_run(last_carousing->success);
		dialog(getnm("Continue"), getnm(ids(last_id, "Success")));
	}
	picture = push_avatar;
	last_id = push;
}

static void apply_carousing(creaturei** characters) {
	carousinga deck; create_source(deck);
	auto push_carousing = last_carousing;
	auto push_player = player;
	for(auto i = 0; i < 6; i++) {
		player = characters[i];
		if(!player || player->isdisabled())
			continue;
		last_carousing = pick(deck);
		if(!last_carousing) {
			create_source(deck);
			last_carousing = pick(deck);
			if(!last_carousing)
				continue;
		}
		apply_carousing();
	}
	player = push_player;
	last_carousing = push_carousing;
	dialog(getnm("Continue"), getnm("CarousingEnd"));
}

void apply_carousing(int bonus) {
	apply_carousing(characters);
}