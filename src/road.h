// Roads (25 Oct 2026, milestone 6's last item, the user's: "very
// visible"), Civilization's: a builder makes one with `r` (ORDERS_ROAD,
// improve.h), every city stands on one, and a step from a road to a road
// costs a third of a move (unit_move_to(), unit.h, which is why moves are
// counted in thirds, MOVE_FULL). On grassland, plains and desert a road
// adds a trade (yield() in growth.c), as there.
//
// **A list sorted by hexagon row, not a layer**, as the resources are
// (map.h): the map byte has no bit left, and the redraw looks at a row's
// few roads instead of testing every hexagon in view. Entries
// road_first[hy] to road_first[hy + 1] - 1 are hexagon row hy's, in column
// order: column road_ax[i], and in road_mask[i] a bit for each DIR_*
// neighbour (map.h) with a road too, which is what is drawn -- a half-road
// from the middle to that edge, TILE_ROAD_FIRST + dir, or TILE_ROAD_DOT for
// a road with none. In HIGH_BSS, saved.
#ifndef SEVENSHIPS_ROAD_H
#define SEVENSHIPS_ROAD_H

#include "mega65.h"
#include "map.h"

// Room for every city and about four roads each of their own -- the
// computer players stop at ROAD_AI_MAX, so that the human always has some.
#define MAX_ROADS 200
#define ROAD_AI_MAX 160
#define ROAD_NONE 0xff

extern byte road_count;
extern byte road_first[2 * MAP_ROWS + 1];
extern byte road_ax[MAX_ROADS];
extern byte road_mask[MAX_ROADS];

// No roads, for a new game.
void road_reset(void);

// The road on (ax, ay), its index in the list, or ROAD_NONE.
byte road_at(byte ax, byte ay);

// A road on (ax, ay), if there is none and the list has room, joined to
// those next to it. **In region 2**, with the builders (improve.c);
// city_found() calls it through the stub.
void road_add(byte ax, byte ay);

#endif // SEVENSHIPS_ROAD_H
