#pragma once

#include "nameable.h"
#include "npc.h"
#include "resid.h"
#include "variant.h"

enum featn : unsigned char;
enum alignmentn : unsigned char;

struct monsteri : nameable {
   resid	res;
   short	frames[4], overlays[4];
   int		experience;
   char		hd, ac;
   alignmentn alignment;
   variants	feats, spells;
   bool		is(featn v) const;
};
