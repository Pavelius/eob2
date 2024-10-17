#pragma once

#include "nameable.h"
#include "npc.h"
#include "resid.h"
#include "variant.h"

struct monsteri : nameable {
   resid	res;
   short	frames[4];
   variants	feats, spells;
};
