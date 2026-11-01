// The fishing boats' turn of work (boat_work(), src/boat.h), in region 8
// with the treasury, which calls it once a turn: region 4, with the rest
// of boat.c, had no room for it (1 Nov 2026).
#include "boat.h"
#include "game.h"
#include "gameint.h"
#include "map.h"
#include "unit.h"

#pragma clang section text="r8text" rodata="r8rodata"

// Turns a fishing boat works fish before the nets are laid, irrigation's.
#define FISH_TURNS 5

void boat_work(void)
{
    byte u, ax, ay, done = 0;

    for (u = 0; u < MAX_UNITS; ++u) {
        if (unit_kind[u] != KIND_BOAT || unit_orders[u] != ORDERS_IRRIGATE)
            continue;
        ax = unit_ax[u];
        ay = unit_ay[u];
        // Another boat on the same fish may have finished it.
        if (map_tile[ay][ax] & MAP_IRRIGATED) {
            unit_orders[u] = ORDERS_NONE;
            continue;
        }
        if (++unit_work[u] < FISH_TURNS) continue;
        map_tile[ay][ax] |= MAP_IRRIGATED;
        unit_orders[u] = ORDERS_NONE;
        if (unit_owner[u] == PLAYER_HUMAN) done = 1;
    }
    if (done) message("Fishing nets", "laid");
}
