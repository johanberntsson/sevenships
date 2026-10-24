// GOTO (25 Oct 2026, the user's, from play: Civilization's): `g`, then a
// hexagon picked with tile select's ring or a click, and the unit walks
// there by itself, a turn's moves at a time, asking for no orders until
// it arrives or finds the way blocked.
//
// **ORDERS_GOTO, and the target in unit_goto_x/y[]** (unit.h, saved). The
// way is path_route() (path.h) for every step, over what the human has
// explored, so a unit goes round what it learns of on the way. It never
// attacks: a step onto another player's unit or city stops it, with its
// orders back, as does a way that is gone. Only the human's units go to;
// the computer players have their own ways about the map (ai.c).
//
// **In region 4** (src/region.h), beside path_route(), which it calls
// directly; called through stubs from game.c and turn.c.
#ifndef SEVENSHIPS_GOTO_H
#define SEVENSHIPS_GOTO_H

#include "mega65.h"

// The human's unit u is to go to (ax, ay), and walks there with the moves
// it has left. If (ax, ay) is unexplored, the wrong terrain for it, or out
// of reach, the panel says so and nothing changes. Afterwards u has
// ORDERS_GOTO while it is on its way, and ORDERS_NONE once there or
// stopped, and the orders go to whichever unit wants them next -- u again
// if it is there with moves left. game.c's, for the active unit.
void goto_order(byte u, byte ax, byte ay);

// Every unit of the human's with ORDERS_GOTO walks on, at the start of a
// turn, once it has its moves back.
void goto_turn(void);

#endif // SEVENSHIPS_GOTO_H
