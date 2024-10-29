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
	CellCocon, CellCoconOpened,
	CellGrave, CellGraveDesecrated,
	CellPitUp, // On floor
	// Decor
	CellPuller, CellSecrectButton, CellCellar, CellMessage,
	CellKeyHole, CellTrapLauncher,
	CellDecor1, CellDecor2, CellDecor3,
	CellDoorButton
};
enum cellfn : unsigned char {
	CellExplored, CellActive, CellExperience,
	Passable,
	LookWall, LookOverlay, LookObject,
	PassableActivated
};
struct celli : nameable, picturei {
	flagable<1, unsigned> flags;
	celln		activate;
};
struct cellfi : nameable {
};
