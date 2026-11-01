// The fishing boats' work and the ships' exploring (milestone 9): a boat
// works fish, Civilization IV's way, and the computer players' boats and
// galleys explore the coasts.
//
// **In region 4** (src/region.h), beside sea_route(), which it calls for
// every step; called from region 1, the AI.
#ifndef SEVENSHIPS_BOAT_H
#define SEVENSHIPS_BOAT_H

#include "mega65.h"

// **Fishing boat u starts working the fish it is on**, as a builder
// starts an improvement (1 Nov 2026, the user's tester: a boat used up
// was not how builders work): ORDERS_IRRIGATE, its moves gone, and
// boat_work() counts its turns. 0, and nothing done, if it is not a
// fishing boat on fish that is not worked yet.
byte boat_fish(byte u);

// A turn's work for every fishing boat at work, from treasury_turn()
// (src/boatwork.c, region 8): after
// FISH_TURNS the fish gets its nets, MAP_IRRIGATED on the shallows (+1
// food, as irrigation is, and TILE_FISH_NET), and **the boat is free
// again**, to explore or work other fish. The panel says so for the
// human's.
void boat_work(void);

// The human's i for fishing boat u, the active unit: boat_fish(), and the
// panel says how it went; the orders go on to the next unit. game.c's.
void boat_order(byte u);

// Ship u, a computer player's, with the moves it has left. **A fishing
// boat** that is not its people's first, or with nothing left to explore,
// makes for the nearest fish next to one of its people's cities that is
// not worked and starts working it (boat_fish()). Otherwise -- the first boat, and
// a galley with nobody to ferry -- **it explores the coasts**: to the
// nearest block of the map with shallows that its people has not seen
// (ai_seen, ai_sea in ai.h), along the shallows by sea_route(). A block it
// cannot reach, or has come next to its shallows, counts as seen. With
// nothing to do it waits. Uses up at least one move.
void boat_explore(byte u);

#endif // SEVENSHIPS_BOAT_H
