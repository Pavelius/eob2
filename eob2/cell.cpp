#include "bsdata.h"
#include "cell.h"

BSDATA(celli) = {
	{"CellUnknown"},
	{"CellPassable"},
	{"CellWall"},
	{"CellDoor"},
	{"CellStairsUp"},
	{"CellStairsDown"},
	{"CellPortal"},
	{"CellButton"},
	{"CellPit"},
	{"CellWeb"},
	{"CellWebTorned"},
	{"CellBarel"},
	{"CellBarelDestroyed"},
	{"CellEyeColumn"},
	{"CellCocon"},
	{"CellCoconOpened"},
	{"CellGrave"},
	{"CellGraveDesecrated"},
	{"CellPitUp"},
	{"CellPuller"},
	{"CellSecrectButton"},
	{"CellCellar"},
	{"CellMessage"},
	{"CellKeyHole"},
	{"CellTrapLauncher"},
	{"CellDecor1"},
	{"CellDecor2"},
	{"CellDecor3"},
	{"CellDoorButton"},
};
assert_enum(celli, CellDoorButton)
BSDATA(cellfi) = {
	{"CellExplored"},
	{"CellActive"},
	{"Passable"},
	{"LookWall"},
	{"LookOverlay"},
	{"LookObject"},
	{"PassableActivated"},
};
assert_enum(cellfi, PassableActivated)
