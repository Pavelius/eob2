#pragma once

#include "nameable.h"
#include "npc.h"
#include "resid.h"
#include "variant.h"

enum featn : unsigned char;

struct monsteri : nameable {
   resid	res;
   short	frames[4], overlays[4];
   char		hd;
   variants	feats, spells;
   bool		is(featn v) const;
};
