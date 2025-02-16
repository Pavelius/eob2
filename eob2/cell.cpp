#include "bsdata.h"
#include "cell.h"
#include "resid.h"
#include "math.h"
#include "view.h"

BSDATA(celli) = {
	{"CellUnknown", NONE, -1, FG(Passable)},
	{"CellPassable", NONE, -1, FG(Passable)},
	{"CellWall", NONE, 0 * walls_frames, FG(LookWall)},
	{"CellDoor", NONE, 2 * walls_frames, FG(LookWall) | FG(PassableActivated)},
	{"CellStairsUp", NONE, 3 * walls_frames, FG(LookWall) | FG(Passable) | FG(MonsterForbidden)},
	{"CellStairsDown", NONE, 4 * walls_frames, FG(LookWall) | FG(Passable) | FG(MonsterForbidden)},
	{"CellPortal", NONE, 5 * walls_frames, FG(LookWall)},
	{"CellButton", NONE, decor_offset + 1 * decor_frames, FG(LookObject) | FG(Passable) | FG(FloorLevel)},
	{"CellPit", NONE, decor_offset + 3 * decor_frames, FG(LookObject) | FG(Passable) | FG(MonsterForbidden) | FG(FloorLevel)},
	{"CellWeb", DECORS, 0 * decor_frames, FG(LookObject), CellWebTorned},
	{"CellWebTorned", DECORS, 1 * decor_frames, FG(LookObject) | FG(Passable)},
	{"CellBarel", DECORS, 3 * decor_frames, FG(LookObject), CellBarelDestroyed},
	{"CellBarelDestroyed", DECORS, 4 * decor_frames, FG(LookObject) | FG(Passable)},
	{"CellEyeColumn", DECORS, 5 * decor_frames, FG(LookObject)},
	{"CellBloodStain", DECORS, 12 * decor_frames, FG(LookObject) | FG(Passable) | FG(FloorLevel)},
	{"CellBloodBlades", DECORS, 10 * decor_frames, FG(LookObject) | FG(Passable) | FG(MonsterForbidden) | FG(FloorLevel)},
	{"CellDirtyStains", DECORS, 13 * decor_frames, FG(LookObject) | FG(Passable) | FG(FloorLevel)},
	{"CellJugDestroyed", DECORS, 11 * decor_frames, FG(LookObject) | FG(Passable)},
	{"CellCocon", DECORS, 6 * decor_frames, FG(LookObject), CellCoconOpened},
	{"CellCoconOpened", DECORS, 7 * decor_frames, FG(LookObject) | FG(Passable)},
	{"CellGrave", DECORS, 8 * decor_frames, FG(LookObject) | FG(Passable), CellGraveDesecrated},
	{"CellGraveDesecrated", DECORS, 9 * decor_frames, FG(LookObject) | FG(Passable) | FG(FloorLevel)},
	{"CellPitUp", NONE, decor_offset + 4 * decor_frames, FG(LookObject) | FG(Passable)},
	{"CellPuller", NONE, decor_offset + 9 * decor_frames, FG(LookOverlay)},
	{"CellSecretButton", NONE, decor_offset + 7 * decor_frames, FG(LookOverlay), CellPassable},
	{"CellCellar", NONE, decor_offset + 11 * decor_frames, FG(LookOverlay)},
	{"CellMessage", NONE, decor_offset + 12 * decor_frames, FG(LookOverlay)},
	{"CellKeyHole", NONE, decor_offset + 13 * decor_frames, FG(LookOverlay)},
	{"CellTrapLauncher", NONE, decor_offset + 15 * decor_frames, FG(LookOverlay)},
	{"CellDecor1", NONE, decor_offset + 16 * decor_frames, FG(LookOverlay)},
	{"CellDecor2", NONE, decor_offset + 17 * decor_frames, FG(LookOverlay)},
	{"CellDecor3", NONE, decor_offset + 18 * decor_frames, FG(LookOverlay)},
	{"CellDoorButton", NONE, -1, FG(LookOverlay)},
	{"CellOverlay1", OVERLAYS, 0, FG(LookWall) | FG(Passable) | FG(MonsterForbidden)},
	{"CellOverlay2", OVERLAYS, 1, FG(LookWall) | FG(Passable) | FG(MonsterForbidden)},
	{"CellOverlay3", OVERLAYS, 2, FG(LookWall) | FG(Passable) | FG(MonsterForbidden)},
};
assert_enum(celli, CellOverlay3)
BSDATA(cellfi) = {
	{"CellExplored"},
	{"CellActive"},
	{"CellExperience"},
	{"Passable"},
	{"MonsterForbidden"},
	{"LookWall"},
	{"LookOverlay"},
	{"LookObject"},
	{"FloorLevel"},
	{"PassableActivated"},
};
assert_enum(cellfi, PassableActivated)
