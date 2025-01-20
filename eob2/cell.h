#pragma once

#include "flagable.h"
#include "nameable.h"
#include "picture.h"

enum resid : unsigned short;

enum celln : unsigned char {
	CellUnknown,
	// Dungeon cells
	CellPassable, CellWall, CellDoor, CellStairsUp, CellStairsDown, CellPortal, // On space
	CellButton, CellPit,
	CellWeb, CellWebTorned,
	CellBarel, CellBarelDestroyed,
	CellEyeColumn,
	CellBloodStain, CellBloodStainBlades, CellJugDestroyed,
	CellCocon, CellCoconOpened,
	CellGrave, CellGraveDesecrated,
	CellPitUp, // On floor
	// Decor
	CellPuller, CellSecretButton, CellCellar, CellMessage,
	CellKeyHole, CellTrapLauncher,
	CellDecor1, CellDecor2, CellDecor3,
	CellDoorButton
};
enum cellfn : unsigned char {
	CellExplored, CellActive, CellExperience,
	Passable, MonsterForbidden,
	LookWall, LookOverlay, LookObject,
	PassableActivated
};
struct celli : nameable, picturei {
	flag32	flags;
	celln	activate;
	bool	is(cellfn v) const { return flags.is(v); }
};
struct cellfi : nameable {
};
