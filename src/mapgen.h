// The map generator (26 Oct 2026, the user's: a new world for every game,
// made on the machine rather than by a script before the disk is built).
//
// **It is tools/mapgen.py, step for step and random number for random
// number**: a seed makes the same world in both, which is what lets the
// Python one preview and tune what the game will make. A change to one is
// a change to the other, checked byte for byte (CLAUDE.md, *The hex grid*).
//
// It needs no memory of its own beyond a row: map_tile's low nibble is the
// terrain, and its high nibble, clear before game_init() and only used by
// the game from then on, holds in turn the distance to the coast, erosion's
// pending change and the resources, which end up in the list (map_res_* in
// src/map.h). In region 9 (src/region.h), called between the title and
// game_init(), and never during a game.
#ifndef SEVENSHIPS_MAPGEN_H
#define SEVENSHIPS_MAPGEN_H

#include "mega65.h"

// The land, in per cent of the map: `make MAP_LAND=45`.
#ifndef MAP_LAND
#define MAP_LAND 35
#endif

// The world's seed: what the last map_new() made, saved with a game and
// shown on World -> Score, so that a world can be made again
// (`make MAP_SEED=n`, `tools/mapgen.py --seed n`).
extern word map_seed;

// A new world in map_tile and the resource list, saying so on the screen
// while it is made: from MAP_SEED if the build defines it (headless tests,
// and NO_TITLE and FAKE_KEYS builds, which the Makefile gives 1), else
// from the MEGA65's random number register and the timers.
void map_new(void);

#endif // SEVENSHIPS_MAPGEN_H
