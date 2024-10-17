#include "feat.h"
#include "monster.h"

bool monsteri::is(featn value) const {
	for(auto v : feats) {
		if(v.iskind<feati>() && v.value == value)
			return true;
	}
	return false;
}