#pragma once

#include "nameable.h"
#include "npc.h"
#include "resid.h"
#include "variant.h"

enum alignmentn : unsigned char;
enum featn : unsigned char;
enum reactions : unsigned char;

struct monsteri : nameable {
   resid	res;
   short	frames[4], overlays[4];
   int		experience;
   char		hd, ac;
   reactions reaction;
   alignmentn alignment;
   variants	feats, spells;
   bool		is(featn v) const;
};
