// The map generator; src/mapgen.h has the arrangement, and
// tools/mapgen.py, which this follows line for line, the steps and why.
// In region 9 (src/region.h), called through the stub in src/dispatch.s
// under the plain name.
#define map_new map_new_r9

#include "mapgen.h"
#include "map.h"
#include "screen.h"
#include "sheet.h"

#pragma clang section text="r9text" rodata="r9rodata"

#define CELLS ((word)MAP_COLUMNS * MAP_ROWS)
#define FAR 15                  // the coast distance is a nibble
#define RIVERS 15
#define RIVER_TRIES 2000
#define TIN_NODES 4
#define TIN_ROWS 3
#define RES_STRIDE 1597         // shares no factor with CELLS
// Rainfall thresholds, in sixteenths.
#define DESERT_RAIN 21
#define DRY_PLAINS_RAIN 30
#define PLAINS_RAIN 16
#define FOREST_RAIN 32

#define TER(X, Y) (map_tile[Y][X] & MAP_TERRAIN_MASK)
#define NIB(X, Y) (map_tile[Y][X] >> 4)
#define SET_TER(X, Y, V) (map_tile[Y][X] = (map_tile[Y][X] & 0xf0) | (V))
#define SET_NIB(X, Y, V) (map_tile[Y][X] = (map_tile[Y][X] & 0x0f) | ((V) << 4))
#define IS_LAND(T) ((T) != TILE_OCEAN && (T) != TILE_SHALLOW_OCEAN)

// Statics of a banked file are resident (CLAUDE.md), so they go out of
// the way: the seed and the generator's state where they last, the row
// and the river where they are rebuilt on every use.
HIGH_BSS word map_seed;
HIGH_BSS static word state;
LOW_FREE static byte rain[MAP_COLUMNS];
LOW_FREE static byte path_x[FAR + 1], path_y[FAR + 1];

// The neighbour map_neighbour() found.
static byte nx, ny;

// The resources each terrain may hold (by TILE_*): the per cent of
// hexagons with one, and where its choices start in res_kind[] and
// res_weight[], with how many and their weights' sum. RESOURCE_TERRAIN in
// mapgen.py, in the same order.
static const byte res_chance[TERRAIN_COUNT] = {
    16, 10, 12, 0, 8, 0, 12, 25, 0, 20
};
static const byte res_first[TERRAIN_COUNT] = {
    0, 5, 8, 0, 10, 0, 12, 15, 0, 16
};
static const byte res_choices[TERRAIN_COUNT] = {
    5, 3, 2, 0, 2, 0, 3, 1, 0, 1
};
static const byte res_total[TERRAIN_COUNT] = {
    83, 4, 5, 0, 4, 0, 5, 1, 0, 1
};
static const byte res_kind[] = {
    RES_CATTLE, RES_OLIVES, RES_WINE, RES_COPPER, RES_TIN,      // hills
    RES_GRAIN, RES_CATTLE, RES_HORSES,                          // grass
    RES_COPPER, RES_TIN,                                        // mountain
    RES_FISH, RES_DYE,                                          // shallows
    RES_CATTLE, RES_WINE, RES_HORSES,                           // plains
    RES_CEDAR,                                                  // forest
    RES_GRAIN                                                   // floodplain
};
static const byte res_weight[] = {
    10, 20, 20, 20, 13,
    1, 2, 1,
    3, 2,
    3, 1,
    2, 1, 2,
    1,
    1
};

// xorshift16 (7, 9, 8), as rnd() in src/combat.c; Rng in mapgen.py.
static word next(void)
{
    word s = state;
    s ^= s << 7;
    s ^= s >> 9;
    s ^= s << 8;
    state = s;
    return s;
}

static word rand(word n)
{
    return next() % n;
}

// map_neighbour() for the six neighbours, in bytes and without the
// pointers, which made a world take a fifth longer. The steps in doubled
// coordinates, DIR_* order, -1 as 255; in steps of a byte each, since a
// narrowing cast inside a wider expression is sign-extended (CLAUDE.md).
static const byte step_dx[6] = { 2, 1, 255, 254, 255, 1 };
static const byte step_dy[6] = { 0, 1, 1, 0, 255, 255 };

static byte step(byte x, byte y, byte d)
{
    byte hy = y << 1;
    hy += x & 1;
    hy += step_dy[d];
    if (hy >= 2 * MAP_ROWS) return 0;       // -1 is 255
    nx = x + step_dx[d];
    nx &= MAP_COLUMNS - 1;
    hy -= nx & 1;
    ny = hy >> 1;
    return 1;
}

static byte lowland(byte t)
{
    return t == TILE_GRASS || t == TILE_PLAINS || t == TILE_FOREST
           || t == TILE_DESERT || t == TILE_TUNDRA;
}

static word count_cells(byte least)
{
    word n = 0;
    byte x, y;
    for (y = 0; y < MAP_ROWS; ++y)
        for (x = 0; x < MAP_COLUMNS; ++x)
            if (NIB(x, y) >= least) ++n;
    return n;
}

static byte cell_x, cell_y;

// A random cell at least `least` from the coast; the caller has made sure
// there is one.
static void any_cell(byte least)
{
    do {
        cell_x = (byte)rand(MAP_COLUMNS);
        cell_y = (byte)rand(MAP_ROWS);
    } while (NIB(cell_x, cell_y) < least);
}

// Step 1.
static void make_land(void)
{
    // In 32 bits: int is 16 here, and 2560 * 35 is not.
    word target = (word)((dword)CELLS * MAP_LAND / 100), count = 0;
    byte ax, ay, cx, cy, walk, d;

    while (count < target) {
        ax = (byte)rand(MAP_COLUMNS);
        ay = (byte)(3 + rand(MAP_ROWS - 6));
        for (walk = (byte)(8 + rand(56)); walk; --walk) {
            // The cell itself, then its neighbours.
            for (d = 0; d < 7; ++d) {
                if (!d) {
                    cx = ax;
                    cy = ay;
                } else if (step(ax, ay, d - 1)) {
                    cx = nx;
                    cy = ny;
                } else continue;
                if (cy >= 2 && cy < MAP_ROWS - 2
                    && TER(cx, cy) == TILE_OCEAN) {
                    SET_TER(cx, cy, TILE_GRASS);
                    ++count;
                }
            }
            if (step(ax, ay, (byte)rand(6)) && ny >= 3 && ny < MAP_ROWS - 3) {
                ax = nx;
                ay = ny;
            }
        }
    }
}

// The steps from each land cell to the nearest water, at most FAR, in the
// high nibble, relaxed until nothing changes: a sweep forwards, then one
// backwards, and so on, which settles in three or four.
static void coast_distance(void)
{
    byte x, y, i, j, k, d, e, changed, back = 0;

    for (y = 0; y < MAP_ROWS; ++y)
        for (x = 0; x < MAP_COLUMNS; ++x) {
            d = TER(x, y);
            SET_NIB(x, y, IS_LAND(d) ? FAR : 0);
        }
    do {
        changed = 0;
        for (j = 0; j < MAP_ROWS; ++j)
            for (i = 0; i < MAP_COLUMNS; ++i) {
                y = back ? MAP_ROWS - 1 - j : j;
                x = back ? MAP_COLUMNS - 1 - i : i;
                d = NIB(x, y);
                if (!d) continue;
                e = d;
                for (k = 0; k < 6; ++k)
                    if (step(x, y, k) && NIB(nx, ny) + 1 < e)
                        e = NIB(nx, ny) + 1;
                if (e != d) {
                    SET_NIB(x, y, e);
                    changed = 1;
                }
            }
        back ^= 1;
    } while (changed);
}

// Step 2: mountain ranges, inland only.
static void raise_mountains(void)
{
    word target, placed = 0, attempts = 0;
    byte x, y, d, n;

    if (!count_cells(2)) return;
    target = count_cells(1) / 12;
    while (placed < target && attempts < 1000) {
        ++attempts;
        any_cell(2);
        x = cell_x;
        y = cell_y;
        d = (byte)rand(6);
        for (n = (byte)(3 + rand(7)); n; --n) {
            if (NIB(x, y) >= 2 && TER(x, y) != TILE_MOUNTAIN) {
                SET_TER(x, y, TILE_MOUNTAIN);
                ++placed;
            }
            if (rand(10) < 3) d = (byte)rand(6);
            if (!step(x, y, d)) break;
            x = nx;
            y = ny;
        }
    }
}

// Step 2, second half: foothills, then short walks.
static void raise_hills(void)
{
    word target, placed = 0, attempts = 0;
    byte x, y, k, near, n;

    for (y = 0; y < MAP_ROWS; ++y)
        for (x = 0; x < MAP_COLUMNS; ++x) {
            if (TER(x, y) != TILE_GRASS) continue;
            near = 0;
            for (k = 0; k < 6; ++k)
                if (step(x, y, k) && TER(nx, ny) == TILE_MOUNTAIN) near = 1;
            if (near && rand(100) < 35) SET_TER(x, y, TILE_HILLS);
        }
    target = count_cells(1) / 24;
    while (placed < target && attempts < 1000) {
        ++attempts;
        any_cell(1);
        x = cell_x;
        y = cell_y;
        for (n = (byte)(2 + rand(4)); n; --n) {
            if (TER(x, y) == TILE_GRASS) {
                SET_TER(x, y, TILE_HILLS);
                ++placed;
            }
            if (step(x, y, (byte)rand(6))) {
                x = nx;
                y = ny;
            }
        }
    }
}

// Steps 3 and 4, a row at a time.
static void climate(void)
{
    byte x, y, i, back, lap, t;
    word moisture, drop, wet;
    int lat, base;

    for (y = 0; y < MAP_ROWS; ++y) {
        for (x = 0; x < MAP_COLUMNS; ++x) rain[x] = 0;
        for (back = 0; back < 2; ++back) {
            moisture = 0;
            for (lap = 0; lap < 2; ++lap)
                for (i = 0; i < MAP_COLUMNS; ++i) {
                    x = back ? (byte)(MAP_COLUMNS - 1 - i) : i;
                    t = TER(x, y);
                    if (!IS_LAND(t)) {
                        moisture += 16;
                        if (moisture > 128) moisture = 128;
                        continue;
                    }
                    drop = t == TILE_MOUNTAIN ? moisture >> 1
                                              : (moisture * 5) >> 5;
                    if (lap) rain[x] += (byte)drop;
                    moisture -= drop;
                }
        }
        base = 2 * (int)y - (MAP_ROWS - 1);
        if (base < 0) base = -base;
        base = base * 100 / (MAP_ROWS - 1);
        for (x = 0; x < MAP_COLUMNS; ++x) {
            if (TER(x, y) != TILE_GRASS) continue;
            lat = base + (int)rand(17) - 8;
            wet = rain[x] + rand(24);
            if (lat > 68) SET_TER(x, y, TILE_TUNDRA);
            else if (lat > 25 && lat < 55) {
                if (wet < DESERT_RAIN) SET_TER(x, y, TILE_DESERT);
                else if (wet < DRY_PLAINS_RAIN) SET_TER(x, y, TILE_PLAINS);
            } else if (wet < PLAINS_RAIN) SET_TER(x, y, TILE_PLAINS);
            else if (wet > FOREST_RAIN) SET_TER(x, y, TILE_FOREST);
        }
    }
}

// Erosion, twice; the change waits in the high nibble, one more than the
// terrain, until the whole map has been looked at.
static void erode(void)
{
    byte round, x, y, k, j, t, u, n;
    byte around[6];             // the neighbours' terrain, 255 for none

    for (round = 0; round < 2; ++round) {
        for (y = 0; y < MAP_ROWS; ++y)
            for (x = 0; x < MAP_COLUMNS; ++x) {
                SET_NIB(x, y, 0);
                t = TER(x, y);
                if (!lowland(t)) continue;
                for (k = 0; k < 6; ++k)
                    around[k] = step(x, y, k) ? TER(nx, ny) : 255;
                for (k = 0; k < 6; ++k) {
                    u = around[k];
                    if (u == 255 || u == t || !lowland(u)) continue;
                    n = 0;
                    for (j = 0; j < 6; ++j)
                        if (around[j] == u) ++n;
                    if (n >= 4) SET_NIB(x, y, u + 1);
                }
            }
        for (y = 0; y < MAP_ROWS; ++y)
            for (x = 0; x < MAP_COLUMNS; ++x) {
                n = NIB(x, y);
                if (n) map_tile[y][x] = n - 1;
            }
    }
}

// Step 5.
static void rivers(void)
{
    byte made = 0, x, y, t, k, n, len, i, dry;
    byte lx[6], ly[6];
    word tries;

    for (tries = 0; tries < RIVER_TRIES && made < RIVERS; ++tries) {
        x = (byte)rand(MAP_COLUMNS);
        y = (byte)rand(MAP_ROWS);
        t = TER(x, y);
        if ((t != TILE_MOUNTAIN && t != TILE_HILLS) || NIB(x, y) < 2)
            continue;
        len = 0;
        while (NIB(x, y)) {
            n = 0;
            for (k = 0; k < 6; ++k)
                if (step(x, y, k) && NIB(nx, ny) < NIB(x, y)) {
                    lx[n] = nx;
                    ly[n] = ny;
                    ++n;
                }
            if (!n) break;      // cannot happen below FAR
            k = (byte)rand(n);
            x = lx[k];
            y = ly[k];
            path_x[len] = x;
            path_y[len] = y;
            ++len;
        }
        dry = 0;
        for (i = 0; i < len; ++i) {
            t = TER(path_x[i], path_y[i]);
            if (t == TILE_DESERT || t == TILE_PLAINS) ++dry;
        }
        if (dry < 2) continue;
        for (i = 0; i < len; ++i) {
            t = TER(path_x[i], path_y[i]);
            if (t == TILE_DESERT || t == TILE_PLAINS)
                SET_TER(path_x[i], path_y[i], TILE_FLOODPLAIN);
        }
        ++made;
    }
}

// Step 6: lakes filled, then the shallows.
static void clean_up(void)
{
    byte x, y, k, all, any;

    for (y = 0; y < MAP_ROWS; ++y)
        for (x = 0; x < MAP_COLUMNS; ++x) map_tile[y][x] &= MAP_TERRAIN_MASK;
    for (y = 0; y < MAP_ROWS; ++y)
        for (x = 0; x < MAP_COLUMNS; ++x) {
            if (TER(x, y) != TILE_OCEAN) continue;
            all = 1;
            for (k = 0; k < 6; ++k)
                if (step(x, y, k) && !IS_LAND(TER(nx, ny))) all = 0;
            if (all) SET_TER(x, y, TILE_GRASS);
        }
    for (y = 0; y < MAP_ROWS; ++y)
        for (x = 0; x < MAP_COLUMNS; ++x) {
            if (TER(x, y) != TILE_OCEAN) continue;
            any = 0;
            for (k = 0; k < 6; ++k)
                if (step(x, y, k) && IS_LAND(TER(nx, ny))) any = 1;
            if (any) SET_TER(x, y, TILE_SHALLOW_OCEAN);
        }
}

static byte free_cell(byte x, byte y)
{
    byte k;
    if (NIB(x, y)) return 0;
    for (k = 0; k < 6; ++k)
        if (step(x, y, k) && NIB(nx, ny)) return 0;
    return 1;
}

// Step 7, into the high nibble.
static void resources(void)
{
    byte placed = 0, count, x, y, t, r, i;
    word tries, c, pick;

    for (tries = 0; tries < 1000 && placed < TIN_NODES; ++tries) {
        x = (byte)rand(MAP_COLUMNS);
        y = (byte)rand(2 * TIN_ROWS);
        if (y >= TIN_ROWS) y += MAP_ROWS - 2 * TIN_ROWS;
        t = TER(x, y);
        if ((t == TILE_MOUNTAIN || t == TILE_HILLS || t == TILE_TUNDRA)
            && free_cell(x, y)) {
            SET_NIB(x, y, RES_TIN);
            ++placed;
        }
    }
    count = placed;

    c = rand(CELLS);
    for (tries = 0; tries < CELLS; ++tries) {
        c += RES_STRIDE;
        if (c >= CELLS) c -= CELLS;
        x = (byte)(c & (MAP_COLUMNS - 1));
        y = (byte)(c / MAP_COLUMNS);
        t = TER(x, y);
        if (count == MAP_RES_MAX || !res_choices[t]) continue;
        if (rand(100) >= res_chance[t] || !free_cell(x, y)) continue;
        pick = rand(res_total[t]);
        i = res_first[t];
        for (r = res_choices[t]; --r; ++i) {
            if (pick < res_weight[i]) break;
            pick -= res_weight[i];
        }
        SET_NIB(x, y, res_kind[i]);
        ++count;
    }
}

// The nibbles to map_res_*, hexagon row by row, and cleared.
static void res_list(void)
{
    byte hy, x, y, n = 0;

    for (hy = 0; hy < 2 * MAP_ROWS; ++hy) {
        map_res_first[hy] = n;
        for (x = hy & 1; x < MAP_COLUMNS; x += 2) {
            y = (byte)((hy - (x & 1)) >> 1);
            if (NIB(x, y)) {
                map_res_ax[n] = x;
                map_res_kind[n] = NIB(x, y);
                ++n;
                map_tile[y][x] &= MAP_TERRAIN_MASK;
            }
        }
    }
    map_res_first[2 * MAP_ROWS] = n;
    map_res_count = n;
}

void map_new(void)
{
    byte x, y;

#ifdef MAP_SEED
    map_seed = MAP_SEED;
#else
    // $D7EF is the MEGA65's random number register; the timers and the
    // raster mix in more, in case an emulator's is weak.
    map_seed = PEEK(0xd7ef);
    map_seed = (map_seed << 8) | PEEK(0xd7ef);
    map_seed ^= ((word)PEEK(0xdc04) << 8) | PEEK(0xd012);
#endif
    screen_clear();
    screen_putsxy(31, 12, "Creating the world", COLOR_LIGHTGREEN);

    state = map_seed ^ 0x2a6d;
    if (!state) state = 1;
    for (y = 0; y < MAP_ROWS; ++y)
        for (x = 0; x < MAP_COLUMNS; ++x) map_tile[y][x] = TILE_OCEAN;
    make_land();
    coast_distance();
    raise_mountains();
    raise_hills();
    climate();
    erode();
    coast_distance();
    rivers();
    clean_up();
    resources();
    res_list();
}
