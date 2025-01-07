#pragma once

#include "damage.h"
#include "dice.h"
#include "nameable.h"
#include "save.h"
#include "variant.h"

struct itemi;

struct trapi : nameable {
	damagen		type;
	dice		damage;
	char		avatar;
	saven		save;
	itemi*		projectile;
	char		targets;
	variants	effect;
};
