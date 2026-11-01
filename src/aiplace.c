// Where the computer players start, out of ai.c into region 0 (21 Oct
// 2026, for room in region 1 for the ferrying), and on to region 8 (25
// Oct 2026, when the peoples started by their own resources and region 0
// had no room): it runs once a game, from game_init() in turn.c, through
// the stub. distance() and land_neighbours() are ai.c's, copied.
#define ai_place ai_place_r8

#include "ai.h"
#include "city.h"
#include "civ.h"
#include "combat.h"
#include "map.h"
#include "minimap.h"
#include "path.h"
#include "sheet.h"
#include "unit.h"

#pragma clang section text="r8text" rodata="r8rodata"

// How far apart the starting settlers should be, in hexagons, and how many
// tries each spacing gets before the next, closer one is accepted.
#define START_SPACING 16
#define START_SPACING_STEP 4
#define START_TRIES 200
#define OWN_SPACING 8           // the closest a start by its own resources may be

#define SEEN_COLUMNS 8          // as ai.c's blocks
#define SEEN_ROWS 4
#define SEEN_X (MAP_COLUMNS / SEEN_COLUMNS)

static word distance(byte ax, byte ay, byte bx, byte by)
{
    int dx = (int)ax - (int)bx;
    int dy = (2 * (int)ay + (ax & 1)) - (2 * (int)by + (bx & 1));
    if (dx < 0) dx = -dx;
    if (dx > MAP_COLUMNS / 2) dx = MAP_COLUMNS - dx;
    if (dy < 0) dy = -dy;
    return (word)(dy + (dx > dy ? (dx - dy) / 2 : 0));
}

static byte land_neighbours(byte ax, byte ay)
{
    byte dir, nx, ny, n = 0, t;
    for (dir = 0; dir < 6; ++dir) {
        if (!map_neighbour(ax, ay, dir, &nx, &ny)) continue;
        t = MAP_TERRAIN(nx, ny);
        if (t != TILE_OCEAN && t != TILE_SHALLOW_OCEAN) ++n;
    }
    return n;
}

// The nearest unit to (ax, ay), anybody's; there are no cities yet.
static word nearest_unit(byte ax, byte ay)
{
    byte i;
    word d, best = 0xffff;
    for (i = 0; i < MAX_UNITS; ++i) {
        if (unit_kind[i] == KIND_NONE) continue;
        d = distance(ax, ay, unit_ax[i], unit_ay[i]);
        if (d < best) best = d;
    }
    return best;
}

// **Each people starts by a resource of its own** (25 Oct 2026, vision.md
// section 3's peoples), two for some: the Mycenaeans' wine and olives, the
// Hittites' copper, the Egyptians' grain, the Canaanites' cedar and dye --
// in CIV_* order.
static const byte own_res[CIVS][2] = {
    { RES_WINE, RES_OLIVES }, { RES_COPPER, RES_COPPER },
    { RES_GRAIN, RES_GRAIN }, { RES_CEDAR, RES_DYE }
};

// Whether a city on (ax, ay) would have one of player p's own resources in
// its reach, the RING.
static byte own(byte p, byte ax, byte ay)
{
    byte dir, nx, ny, r, civ = player_civ[p];
    for (dir = 0; dir < RING; ++dir) {
        if (!map_neighbour(ax, ay, dir, &nx, &ny)) continue;
        r = map_resource_at(nx, ny);
        if (r != RES_NONE && (r == own_res[civ][0] || r == own_res[civ][1]))
            return 1;
    }
    return 0;
}

// **Nobody starts alone if it can be helped** (1 Nov 2026, the user's: a
// save on world 34013 had all four peoples on land masses of their own,
// the Mycenaeans on an island of 46 hexagons, and the human met nobody's
// army until late in the game). The human's start used to be the
// grassland nearest the middle of the first view, which on a new world
// every game is wherever that happens to be. So the land masses are
// flooded first (masses()), and **the human starts on the largest**, and
// **the first computer player on the same one**, room permitting; the
// others on any land mass of START_MASS hexagons or more. Of worlds
// 1-1000 the largest was 249 hexagons at the least.
#define START_MASS 100

// Marks in the map byte while ai_place() runs, cleared before it returns:
// the improvement bits, which nothing has set yet at a game's start.
#define LARGEST MAP_IRRIGATED   // the largest land mass
#define BIG MAP_MINED           // a land mass of START_MASS or more

#define LAND(T) ((T) != TILE_OCEAN && (T) != TILE_SHALLOW_OCEAN)

// Marks the land mass of (ax, ay) with `bit`, breadth first over the land
// hexagons without it, through path.c's queue, and says how many it
// marked. The queue is PATH_QUEUE cells and a hexagon not queued for want
// of room would be left out with what lies beyond it, but of worlds 1-1000
// no flood had more than 94 waiting.
static word flood(byte ax, byte ay, byte bit)
{
    byte head = 0, tail = 1, waiting = 1, dir, nx, ny, t;
    word n = 1;

    map_tile[ay][ax] |= bit;
    path_queue_x[0] = ax;
    path_queue_y[0] = ay;
    while (waiting) {
        ax = path_queue_x[head];
        ay = path_queue_y[head];
        if (++head == PATH_QUEUE) head = 0;
        --waiting;
        for (dir = 0; dir < 6; ++dir) {
            if (!map_neighbour(ax, ay, dir, &nx, &ny)) continue;
            t = map_tile[ny][nx];
            if ((t & bit) || !LAND(t & MAP_TERRAIN_MASK)
                || waiting == PATH_QUEUE)
                continue;
            map_tile[ny][nx] = t | bit;
            ++n;
            path_queue_x[tail] = nx;
            path_queue_y[tail] = ny;
            if (++tail == PATH_QUEUE) tail = 0;
            ++waiting;
        }
    }
    return n;
}

static void unmark(byte bits)
{
    byte ax, ay;
    for (ay = 0; ay < MAP_ROWS; ++ay)
        for (ax = 0; ax < MAP_COLUMNS; ++ax)
            map_tile[ay][ax] &= (byte)~bits;
}

// Every land mass flooded with LARGEST to measure it, and again with BIG
// if it is large enough; then LARGEST taken off and put back on the
// largest alone.
static void masses(void)
{
    byte ax, ay, bx = 0, by = 0;
    word n, best = 0;

    for (ay = 0; ay < MAP_ROWS; ++ay)
        for (ax = 0; ax < MAP_COLUMNS; ++ax) {
            if (!LAND(MAP_TERRAIN(ax, ay)) || (map_tile[ay][ax] & LARGEST))
                continue;
            n = flood(ax, ay, LARGEST);
            if (n >= START_MASS) flood(ax, ay, BIG);
            if (n > best) {
                best = n;
                bx = ax;
                by = ay;
            }
        }
    unmark(LARGEST);
    flood(bx, by, LARGEST);
}

// Where player p starts: grassland with four land neighbours and nobody
// on it, as far from every unit as can be had, on a land mass marked
// `want`, by one of p's own resources first; then without the resource;
// then anywhere. 0 if nowhere at all.
static byte place(byte p, byte want, byte *rx, byte *ry)
{
    byte ax, ay, tries, pass;
    word spacing;

    for (pass = 0; pass < 3; ++pass)
        for (spacing = START_SPACING;; spacing -= START_SPACING_STEP) {
            for (tries = 0; tries < START_TRIES; ++tries) {
                ax = rnd() & (MAP_COLUMNS - 1);
                ay = 2 + rnd() % (MAP_ROWS - 4);    // not the edge rows
                if (MAP_TERRAIN(ax, ay) == TILE_GRASS
                    && !(map_tile[ay][ax] & MAP_OCCUPIED)
                    && (pass == 2 || (map_tile[ay][ax] & want))
                    && land_neighbours(ax, ay) >= 4
                    && nearest_unit(ax, ay) >= spacing
                    && (pass || own(p, ax, ay))) {
                    *rx = ax;
                    *ry = ay;
                    return 1;
                }
            }
            if (spacing <= (pass ? START_SPACING_STEP : OWN_SPACING))
                break;                              // no room: the next pass
        }
    return 0;
}

// **The human's settler and warrior** are where game_init() put them, on
// the grassland nearest the middle of the first view, and go to their
// start from there. **Not in a NO_TITLE build**: the headless scrolling
// check (CLAUDE.md, "Verifying a change") needs the settler in the first
// view, at START_OFF_X's grassland, and every test script in CLAUDE.md
// has it there.
#ifndef NO_TITLE
static void move_human(byte bx, byte by)
{
    byte ax = unit_ax[0], ay = unit_ay[0], x, y, u;

    for (u = 0; u < 2; ++u) {              // the settler and the warrior
        unit_ax[u] = bx;
        unit_ay[u] = by;
    }
    unit_mark(ax, ay);
    unit_mark(bx, by);
    // Unseen again where they were put first, so that the minimap has no
    // patch of it apart; then seen where they are.
    for (u = 0; u <= RING_INNER; ++u) {
        x = ax;
        y = ay;
        if (u < RING_INNER && !map_neighbour(ax, ay, u, &x, &y)) continue;
        map_tile[y][x] &= (byte)~MAP_EXPLORED;
        minimap_plot(x, y);
    }
    map_explore(bx, by);
}
#endif

void ai_place(void)
{
    byte p, ax, ay, t;

    // Nobody has been anywhere yet; and where the land is.
    for (ax = 0; ax < SEEN_X; ++ax) {
        for (p = 0; p < PLAYERS; ++p) ai_seen[p][ax] = 0;
        ai_land[ax] = 0;
        ai_sea[ax] = 0;
    }
    for (ay = 0; ay < MAP_ROWS; ++ay)
        for (ax = 0; ax < MAP_COLUMNS; ++ax) {
            t = MAP_TERRAIN(ax, ay);
            if (t == TILE_SHALLOW_OCEAN)
                ai_sea[ax / SEEN_COLUMNS] |= (byte)(1 << (ay / SEEN_ROWS));
            else if (t != TILE_OCEAN)
                ai_land[ax / SEEN_COLUMNS] |= (byte)(1 << (ay / SEEN_ROWS));
        }

    masses();
#ifndef NO_TITLE
    if (place(PLAYER_HUMAN, LARGEST, &ax, &ay)) move_human(ax, ay);
#endif
    for (p = PLAYER_FIRST_AI; p < PLAYERS; ++p)
        if (place(p, p == PLAYER_FIRST_AI ? LARGEST : BIG, &ax, &ay)) {
            unit_create(KIND_SETTLER, p, ax, ay);
            unit_create(KIND_WARRIOR, p, ax, ay);   // as the human's, game.c
        }
    unmark(LARGEST | BIG);
}
