// In region 4 (src/region.h): called through the stub in src/dispatch.s,
// under this name here.
#define sea_route sea_route_r4

#include "path.h"
#include "map.h"
#include "road.h"
#include "sheet.h"
#include "unit.h"

#pragma clang section text="r4text" rodata="r4rodata"

// A bit a cell; a bit a road (road.h), set once a road's hexagon is
// settled; and four lists of cells to visit, one for each cost in thirds
// of a move from d to d + 3 (a step from a road to a road costs 1, any
// other 3), linked through one pool of 255, byte-indexed. A plain search
// over land never has more than 56 cells waiting on these maps (every
// target, seeds 1, 3, 5 and 7), and the roads add a few more; one that
// fills the pool gives up, as if there were no way.
#define BUCKETS 4
#define END 0xff                // the end of a list, and the pool full
#define FROM 0x80               // in queue_x: the unit's hexagon, and the step
LOW_FREE static byte visited[MAP_ROWS * MAP_COLUMNS / 8];
LOW_FREE static byte road_done[MAX_ROADS / 8 + 1];
LOW_FREE static byte queue_x[END], queue_y[END], queue_next[END];
LOW_FREE static byte first[BUCKETS], last[BUCKETS];
LOW_FREE static byte fresh, spare;      // never used yet; used and given back

static byte push(byte b, byte x, byte y)
{
    byte e = spare;
    if (e != END) spare = queue_next[e];
    else if (fresh != END) e = fresh++;
    else return 0;
    queue_x[e] = x;
    queue_y[e] = y;
    queue_next[e] = END;
    if (first[b] == END) first[b] = e;
    else queue_next[last[b]] = e;
    last[b] = e;
    return 1;
}

// Whether road r's hexagon was settled already, and from now on it is.
static byte done(byte r)
{
    byte bit = (byte)(1 << (r & 7));
    byte *at = road_done + (r >> 3);
    if (*at & bit) return 1;
    *at |= bit;
    return 0;
}

static byte seen(byte ax, byte ay)
{
    word i = (word)ay * MAP_COLUMNS + ax;
    byte bit = (byte)(1 << (i & 7));
    byte *at = visited + (i >> 3);
    if (*at & bit) return 1;
    *at |= bit;
    return 0;
}

// What the search may go through: for a ship the shallows, for a land
// unit land, and for a chariot not mountains -- with `fog`, only what the
// human has explored.
static byte ship, chariot, fog;

static byte passable(byte ax, byte ay)
{
    byte t = map_tile[ay][ax];
    byte terrain = t & MAP_TERRAIN_MASK;

    if (fog && !((t | map_show) & MAP_EXPLORED)) return 0;
    if (ship) return terrain == TILE_SHALLOW_OCEAN;
    if (terrain == TILE_OCEAN || terrain == TILE_SHALLOW_OCEAN) return 0;
    return !chariot || terrain != TILE_MOUNTAIN;
}

byte sea_route(byte fx, byte fy, byte tx, byte ty)
{
    return path_route(fx, fy, tx, ty, KIND_GALLEY, 0);
}

// **Dijkstra's search, from the target out, in thirds of a move**, so that
// GOTO follows the roads (26 Oct 2026, the user's: it walked beside them).
// The costs are 1 and 3, so the cells still to visit are never more than 3
// beyond the nearest, and four rings by cost modulo 4 are the priority
// queue. A hexagon off the roads is reached only by steps of 3, the
// most any cell waiting can cost, so the first time is the best and it is
// queued once (visited); a road's hexagon may be queued again, cheaper,
// by a step along the road, and is settled the first time it comes out
// (road_done). The unit's own hexagon goes into the rings as a step, the
// direction in queue_y, and the first to come out is the cheapest way.
// The lists are visited in turn, d modulo 4, each until it is empty: a
// step from d goes onto d + 1 or d + 3, never onto d itself.
// For a ship every step costs 3, and it is the breadth-first search it was.
byte path_route(byte fx, byte fy, byte tx, byte ty, byte kind, byte explored_only)
{
    byte b = 0, idle = 0, dir, x, y, nx, ny, r, nr, cost, e;
    word i;

    if ((fx == tx && fy == ty) || map_direction(fx, fy, tx, ty) != DIR_NONE)
        return DIR_NONE;
    ship = KIND_IS_SHIP(kind);
    chariot = kind == KIND_CHARIOT;
    fog = explored_only;
    // One loop: Calypsi 5.18 has started a second loop over the same
    // counter from a stale register (CLAUDE.md, road_add()).
    for (i = 0; i < sizeof visited; ++i) {
        visited[i] = 0;
        if (i < sizeof road_done) road_done[i] = 0;
        if (i < BUCKETS) first[i] = END;
    }
    fresh = 0;
    spare = END;
    seen(tx, ty);
    push(0, tx, ty);
    for (;;) {
        e = first[b];
        if (e == END) {
            if (++idle == BUCKETS) break;           // nothing left to visit
            b = (b + 1) & (BUCKETS - 1);
            continue;
        }
        idle = 0;
        first[b] = queue_next[e];
        queue_next[e] = spare;
        spare = e;
        x = queue_x[e];
        y = queue_y[e];
        if (x & FROM) return y;
        r = ship ? ROAD_NONE : road_at(x, y);
        if (r != ROAD_NONE && done(r)) continue;
        for (dir = 0; dir < 6; ++dir) {
            if (!map_neighbour(x, y, dir, &nx, &ny)) continue;
            cost = 3;
            if (r != ROAD_NONE && ((road_mask[r] >> dir) & 1)) cost = 1;
            if (nx == fx && ny == fy) {
                if (!push((b + cost) & (BUCKETS - 1), FROM,
                          map_direction(fx, fy, x, y)))
                    return DIR_NONE;
                continue;
            }
            if (!passable(nx, ny)) continue;
            if (cost == 1) {
                // Along a road: unless that hexagon is settled already.
                nr = road_at(nx, ny);
                if (road_done[nr >> 3] & (byte)(1 << (nr & 7))) continue;
                seen(nx, ny);
            } else if (seen(nx, ny)) {
                continue;
            }
            if (!push((b + cost) & (BUCKETS - 1), nx, ny)) return DIR_NONE;
        }
    }
    return DIR_NONE;
}
