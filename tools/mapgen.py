#!/usr/bin/env python3
"""The map generator of Seven Ships of the Enemy, as the PC sees it.

    mapgen.py [--seed N] [--land PERCENT] [--png preview.png] [out.bin]

**The game makes its own worlds** (src/mapgen.c, since 26 Oct 2026): a new
one for every Start Game, from a seed. This is the same generator, step for
step and random number for random number, so that a seed gives the same
world here as on the MEGA65 -- which is what makes it useful: a preview
(--png) and the resource counts in a moment, for tuning, rather than a
game started in xemu. **The two have to be kept in step**, and a change
to one is checked against the other byte for byte (CLAUDE.md, *The hex
grid*). Hence everything here is integers, one 16-bit xorshift, no lists
picked from (cells are drawn at random until one fits) and the
neighbours in the game's DIR_* order.

The steps follow the Civilization 1 generator described in design.md (and the
civfanatics thread it links), adapted to a hex grid and to the ten terrains
the sheet has today:

1. Land masses: random walks that stamp a hexagon and its six neighbours,
   repeated until the land target is reached. **The map wraps east-west**,
   as the game's view does, so land may run across the seam; it is kept two
   cells away from the top and bottom only.
2. Relief: shorter random walks that raise mountain ranges, only on land at
   least two steps from the coast, so ranges never sit on a beach; then hills,
   as foothills round the ranges and as a few short walks of their own.
3. Rainfall: wind blows along each row, picking moisture up over water and
   dropping it over land, more of it against mountains; done west to east and
   east to west and summed. Each pass goes round the row twice and keeps the
   second lap, so that the seam gets the moisture the wrap would bring it.
   In the warm band dry land becomes desert, then plains, then grassland as
   the rain rises; elsewhere plains, grassland and, wettest, forest.
   Moisture is in sixteenths, a byte.
4. Temperature: land near the top and bottom of the map becomes tundra, with
   a ragged edge rather than a straight line. Then two rounds of erosion.
5. Rivers: from mountains and hills down the coast distance to the sea.
   They are not drawn -- the sheet has no river tile yet -- but where one
   crosses desert or plains it leaves a line of floodplain, the Nile's
   arrangement and the grain the game's peoples lived on.
6. Clean-up: ocean next to land becomes shallow ocean. There are no
   lakes: water cut off from the world ocean is made land straight after
   step 1 (fill_lakes()), since a city by a lake built boats that could
   go nowhere. Then the shortest run of open sea between each area of
   land and shallows and the rest is made shallows too (join_seas()), so
   that a ship can reach every land mass.
7. Resources (milestone 7, vision.md section 2): tin first, a few nodes in
   the far north and south (the imports from the edge of the world), then
   the rest by terrain, a little more tin among them, never next to each
   other; see RESOURCE_TERRAIN.

**Memory on the MEGA65 is map_tile alone**: the terrain is its low nibble,
and the high one, clear until the game starts, holds in turn the distance
to the coast (cut at 15), erosion's pending change and the resources. The
order of the steps here follows from that: the distance is worked out
again after erosion, and cleared before the resources.

The output (when a file is named) is what the game holds afterwards, for
comparing with a -dumpmem: map_tile, 2560 bytes row-major [ay][ax], then
map_res_count, map_res_first (41 bytes), map_res_ax and map_res_kind
(map_res_count bytes each).

The coordinates are the game's: array cell (ax, ay) is the hexagon
(ax, 2*ay + (ax & 1)) in doubled coordinates; see src/map.h.
"""

import argparse
import struct
import sys
import zlib

# MAP_COLUMNS and MAP_ROWS in src/map.h. Keep the two in step.
COLUMNS = 128
ROWS = 20
CELLS = COLUMNS * ROWS

# TILE_* in src/sheet.h. Keep the two in step.
(HILLS, GRASS, MOUNTAIN, OCEAN, SHALLOW_OCEAN, TUNDRA,
 PLAINS, FOREST, DESERT, FLOODPLAIN) = range(10)

# Preview colours, one per terrain value.
COLOURS = {
    HILLS: (120, 96, 48),
    GRASS: (64, 150, 48),
    MOUNTAIN: (130, 130, 140),
    OCEAN: (40, 60, 160),
    SHALLOW_OCEAN: (70, 120, 210),
    TUNDRA: (200, 215, 220),
    PLAINS: (150, 140, 56),
    FOREST: (16, 80, 24),
    DESERT: (240, 228, 140),
    FLOODPLAIN: (150, 210, 80),
}

# RES_* in src/map.h. Keep the two in step.
(NO_RESOURCE, GRAIN, FISH, CATTLE, OLIVES, WINE, CEDAR, COPPER, TIN,
 DYE, HORSES) = range(11)
# What each terrain may hold: (per cent of hexagons with one, [(resource,
# weight), ...]). Tin is about two thirds as common as copper inland, so
# that bronze is possible but scarce before there is trade, and
# resources() adds TIN_NODES more at the edges of the world. res_chance[]
# and res_choice[] in src/mapgen.c.
RESOURCE_TERRAIN = {
    HILLS: (16, [(CATTLE, 10), (OLIVES, 20), (WINE, 20), (COPPER, 20),
                 (TIN, 13)]),
    GRASS: (10, [(GRAIN, 1), (CATTLE, 2), (HORSES, 1)]),
    MOUNTAIN: (12, [(COPPER, 3), (TIN, 2)]),
    SHALLOW_OCEAN: (8, [(FISH, 3), (DYE, 1)]),
    PLAINS: (12, [(CATTLE, 2), (WINE, 1), (HORSES, 2)]),
    FOREST: (25, [(CEDAR, 1)]),     # 12 left 4 a map, too few for galleys
    FLOODPLAIN: (20, [(GRAIN, 1)]),
}
# MAP_RES_MAX in src/map.h: the list's count is a byte.
MAX_RESOURCES = 255
# Tin: this many nodes, on land within TIN_ROWS rows of the top or bottom.
TIN_NODES = 4
TIN_ROWS = 3
# Preview colours, one per resource: a dot in the hexagon.
RESOURCE_COLOURS = {
    GRAIN: (255, 220, 0), FISH: (0, 255, 255), CATTLE: (255, 255, 255),
    OLIVES: (40, 40, 0), WINE: (160, 0, 200), CEDAR: (0, 40, 30),
    COPPER: (255, 120, 0), TIN: (200, 200, 230), DYE: (255, 0, 160),
    HORSES: (140, 90, 40),
}

# Rainfall thresholds, in sixteenths; see climate(). In the warm band, below
# DESERT_RAIN is desert and below DRY_PLAINS_RAIN plains; elsewhere below
# PLAINS_RAIN is plains and above FOREST_RAIN forest. The rest is grassland.
DESERT_RAIN = 21
DRY_PLAINS_RAIN = 30
PLAINS_RAIN = 16
FOREST_RAIN = 32
# Rivers to try for, and how many hexagons to draw looking for sources.
RIVERS = 15
RIVER_TRIES = 2000
# The coast distance is a nibble.
FAR = 15

# The six neighbours in doubled coordinates, DIR_* order (src/map.h):
# east, south-east, south-west, west, north-west, north-east.
HEX_STEPS = ((2, 0), (1, 1), (-1, 1), (-2, 0), (-1, -1), (1, -1))


class Rng:
    """xorshift16 (7, 9, 8), as rnd() in src/combat.c, but a state of its
    own: gen_next() and gen_rand() in src/mapgen.c."""

    def __init__(self, seed):
        self.s = (seed ^ 0x2a6d) & 0xFFFF or 1

    def next(self):
        s = self.s
        s ^= (s << 7) & 0xFFFF
        s ^= s >> 9
        s ^= (s << 8) & 0xFFFF
        self.s = s
        return s

    def rand(self, n):
        """0 to n - 1; n at most 65535."""
        return self.next() % n


def step(ax, ay, d):
    """map_neighbour(): the cell one step in direction d, or None off the
    top or bottom. COLUMNS is even, so wrapping keeps parity."""
    dx, dy = HEX_STEPS[d]
    hx, hy = ax + dx, 2 * ay + (ax & 1) + dy
    if hy < 0 or hy >= 2 * ROWS:
        return None
    hx %= COLUMNS
    return hx, (hy - (hx & 1)) >> 1


class Map:
    def __init__(self):
        self.t = [[OCEAN] * COLUMNS for _ in range(ROWS)]

    def terrain(self, x, y):
        return self.t[y][x] & 15

    def set_terrain(self, x, y, v):
        self.t[y][x] = (self.t[y][x] & 0xF0) | v

    def nib(self, x, y):
        return self.t[y][x] >> 4

    def set_nib(self, x, y, v):
        self.t[y][x] = (self.t[y][x] & 15) | (v << 4)

    def land(self, x, y):
        t = self.terrain(x, y)
        return t != OCEAN and t != SHALLOW_OCEAN


def make_land(rng, m, land_percent):
    """Step 1."""
    target = CELLS * land_percent // 100
    count = 0
    while count < target:
        ax = rng.rand(COLUMNS)
        ay = 3 + rng.rand(ROWS - 6)
        for _ in range(8 + rng.rand(56)):
            for d in range(-1, 6):
                cell = (ax, ay) if d < 0 else step(ax, ay, d)
                if cell is None:
                    continue
                cx, cy = cell
                if 2 <= cy < ROWS - 2 and m.terrain(cx, cy) == OCEAN:
                    m.set_terrain(cx, cy, GRASS)
                    count += 1
            cell = step(ax, ay, rng.rand(6))
            if cell and 3 <= cell[1] < ROWS - 3:
                ax, ay = cell


def coast_distance(m):
    """The steps from each land cell to the nearest water, at most FAR, in
    the high nibble: relaxed until nothing changes, which gives the same
    distances as a breadth-first search without its queue, sweeping
    forwards and backwards in turn."""
    for y in range(ROWS):
        for x in range(COLUMNS):
            m.set_nib(x, y, FAR if m.land(x, y) else 0)
    changed = True
    back = False
    while changed:
        changed = False
        for j in range(ROWS):
            for i in range(COLUMNS):
                y = ROWS - 1 - j if back else j
                x = COLUMNS - 1 - i if back else i
                d = m.nib(x, y)
                if not d:
                    continue
                for k in range(6):
                    cell = step(x, y, k)
                    if cell and m.nib(*cell) + 1 < d:
                        d = m.nib(*cell) + 1
                if d != m.nib(x, y):
                    m.set_nib(x, y, d)
                    changed = True
        back = not back


def any_cell(rng, m, least):
    """A random cell at least `least` from the coast; the caller has made
    sure there is one."""
    while True:
        x = rng.rand(COLUMNS)
        y = rng.rand(ROWS)
        if m.nib(x, y) >= least:
            return x, y


def count_cells(m, least):
    return sum(1 for y in range(ROWS) for x in range(COLUMNS)
               if m.nib(x, y) >= least)


def raise_mountains(rng, m):
    """Step 2: mountain ranges, inland only. About one land cell in twelve."""
    if not count_cells(m, 2):
        return
    target = count_cells(m, 1) // 12
    placed = 0
    attempts = 0
    while placed < target and attempts < 1000:
        attempts += 1
        x, y = any_cell(rng, m, 2)
        # A range has a heading, so it reads as a ridge rather than a blob.
        d = rng.rand(6)
        for _ in range(3 + rng.rand(7)):
            if m.nib(x, y) >= 2 and m.terrain(x, y) != MOUNTAIN:
                m.set_terrain(x, y, MOUNTAIN)
                placed += 1
            # Mostly keep the heading, sometimes wander one step off it.
            if rng.rand(10) < 3:
                d = rng.rand(6)
            cell = step(x, y, d)
            if not cell:
                break
            x, y = cell


def raise_hills(rng, m):
    """Step 2, second half: foothills beside the ranges, then short walks.
    About one land cell in twelve again, all told."""
    for y in range(ROWS):
        for x in range(COLUMNS):
            if m.terrain(x, y) != GRASS:
                continue
            near = False
            for k in range(6):
                cell = step(x, y, k)
                if cell and m.terrain(*cell) == MOUNTAIN:
                    near = True
            if near and rng.rand(100) < 35:
                m.set_terrain(x, y, HILLS)
    target = count_cells(m, 1) // 24
    placed = 0
    attempts = 0
    while placed < target and attempts < 1000:
        attempts += 1
        x, y = any_cell(rng, m, 1)
        for _ in range(2 + rng.rand(4)):
            if m.terrain(x, y) == GRASS:
                m.set_terrain(x, y, HILLS)
                placed += 1
            cell = step(x, y, rng.rand(6))
            if cell:
                x, y = cell


def climate(rng, m):
    """Steps 3 and 4, a row at a time: its rainfall from two passes of wind,
    then its grassland made desert, plains, forest or tundra. The desert
    band sits between the tropics and the cold, and the noise on both
    thresholds is what makes the edges ragged."""
    for y in range(ROWS):
        rain = [0] * COLUMNS
        for back in (0, 1):
            moisture = 0
            for lap in (0, 1):
                for i in range(COLUMNS):
                    x = COLUMNS - 1 - i if back else i
                    if not m.land(x, y):
                        moisture = min(moisture + 16, 128)
                        continue
                    if m.terrain(x, y) == MOUNTAIN:
                        drop = moisture >> 1
                    else:
                        drop = (moisture * 5) >> 5
                    if lap:
                        rain[x] += drop
                    moisture -= drop
        # Latitude in hundredths, 0 at the equator, 100 at either pole.
        base = abs(2 * y - (ROWS - 1)) * 100 // (ROWS - 1)
        for x in range(COLUMNS):
            if m.terrain(x, y) != GRASS:
                continue
            lat = base + rng.rand(17) - 8
            wet = rain[x] + rng.rand(24)
            if lat > 68:
                m.set_terrain(x, y, TUNDRA)
            elif 25 < lat < 55:
                if wet < DESERT_RAIN:
                    m.set_terrain(x, y, DESERT)
                elif wet < DRY_PLAINS_RAIN:
                    m.set_terrain(x, y, PLAINS)
            elif wet < PLAINS_RAIN:
                m.set_terrain(x, y, PLAINS)
            elif wet > FOREST_RAIN:
                m.set_terrain(x, y, FOREST)


def lowland(t):
    """What the climate decides, and what erosion may turn into another."""
    return t in (GRASS, PLAINS, FOREST, DESERT, TUNDRA)


def erode(m):
    """Roughly Civ 1's "age" step: a cell most of whose neighbours are one
    other lowland terrain becomes that terrain. Without it the rainfall
    noise leaves the desert band speckled with single grass cells. The
    change waits in the high nibble, one more than the terrain, until the
    whole map has been looked at. At most one terrain can have four of six
    neighbours."""
    for _ in range(2):
        for y in range(ROWS):
            for x in range(COLUMNS):
                m.set_nib(x, y, 0)
                t = m.terrain(x, y)
                if not lowland(t):
                    continue
                around = [m.terrain(*cell) if cell else None
                          for cell in (step(x, y, k) for k in range(6))]
                for u in around:
                    if u is None or u == t or not lowland(u):
                        continue
                    if around.count(u) >= 4:
                        m.set_nib(x, y, u + 1)
        for y in range(ROWS):
            for x in range(COLUMNS):
                if m.nib(x, y):
                    m.set_terrain(x, y, m.nib(x, y) - 1)
                    m.set_nib(x, y, 0)


def rivers(rng, m):
    """Step 5: each river runs from a mountain or hill to the sea, always
    to a neighbour nearer the coast, and turns the desert and plains on its
    way into floodplain. Only rivers that cross some of either are kept."""
    made = 0
    for _ in range(RIVER_TRIES):
        if made >= RIVERS:
            break
        x = rng.rand(COLUMNS)
        y = rng.rand(ROWS)
        if m.terrain(x, y) not in (MOUNTAIN, HILLS) or m.nib(x, y) < 2:
            continue
        path = []
        while m.nib(x, y):
            lower = []
            for k in range(6):
                cell = step(x, y, k)
                if cell and m.nib(*cell) < m.nib(x, y):
                    lower.append(cell)
            if not lower:
                break   # land FAR or more from the sea, where the nibble ends
            x, y = lower[rng.rand(len(lower))]
            path.append((x, y))
        dry = [(px, py) for px, py in path
               if m.terrain(px, py) in (DESERT, PLAINS)]
        if len(dry) < 2:
            continue
        for px, py in dry:
            m.set_terrain(px, py, FLOODPLAIN)
        made += 1


def fill_lakes(m):
    """Step 1, second half: **water that does not reach the world ocean
    becomes land**, before the climate decides what land it is. A lake
    made a city beside it coastal (city_coastal() looks for shallows
    next to it), and the boats it built could go nowhere. The ocean is
    found from the top row, which is never land, by marking water next to
    marked water in the high nibble, sweeping forwards and backwards in
    turn until nothing changes."""
    for x in range(COLUMNS):
        m.set_nib(x, 0, 1)
    changed = True
    back = False
    while changed:
        changed = False
        for j in range(ROWS):
            for i in range(COLUMNS):
                y = ROWS - 1 - j if back else j
                x = COLUMNS - 1 - i if back else i
                if m.nib(x, y) or m.terrain(x, y) != OCEAN:
                    continue
                for k in range(6):
                    cell = step(x, y, k)
                    if cell and m.nib(*cell):
                        m.set_nib(x, y, 1)
                        changed = True
                        break
        back = not back
    for y in range(ROWS):
        for x in range(COLUMNS):
            if m.terrain(x, y) == OCEAN and not m.nib(x, y):
                m.set_terrain(x, y, GRASS)
            m.set_nib(x, y, 0)


def clean_up(m):
    """Step 6: the shallows. (Lakes went in fill_lakes().)"""
    for y in range(ROWS):
        for x in range(COLUMNS):
            m.set_nib(x, y, 0)
    for y in range(ROWS):
        for x in range(COLUMNS):
            if m.terrain(x, y) == OCEAN and any(
                    m.land(*cell) for cell in
                    (step(x, y, k) for k in range(6)) if cell):
                m.set_terrain(x, y, SHALLOW_OCEAN)


def join_seas(m):
    """Step 6, second half: **every land mass is reachable by ship**. The
    ships keep to the shallows, which are one ring round the land, so a land
    mass further than two hexagons from the others was cut off for good: a
    people starting there met nobody (seed 12876, 30 Oct 2026). So the area
    of land and shallows joined so far is found from the first cell of it,
    the open sea's distance from it measured, and the shortest run of open
    sea to the nearest area not joined yet made shallows; again, until
    every area is joined. All in the high nibble: 1 for the joined area,
    d + 1 for the open sea d steps from it, 0 for anything not reached;
    relaxed, sweeping forwards and backwards in turn, as fill_lakes() is.
    A strait made is marked joined and the relaxing goes on from there
    rather than from the start: the distances only fall, so it settles
    where a fresh start would, in a fraction of the sweeps.
    A gap wider than FAR - 2 is left open (none in seeds 1-2000, where
    1-5 joins were needed in 57% of worlds)."""
    start = next((x, y) for y in range(ROWS) for x in range(COLUMNS)
                 if m.terrain(x, y) != OCEAN)
    joins = 0
    for y in range(ROWS):
        for x in range(COLUMNS):
            m.set_nib(x, y, 0)
    m.set_nib(*start, 1)
    while True:
        changed = True
        back = False
        while changed:
            changed = False
            # Each row both ways before the next, which carries the
            # flood along a winding coast in half the sweeps.
            for j in range(ROWS):
                y = ROWS - 1 - j if back else j
                for i in range(2 * COLUMNS):
                    x = i if i < COLUMNS else 2 * COLUMNS - 1 - i
                    d = m.nib(x, y)
                    if d == 1:
                        continue
                    sea = m.terrain(x, y) == OCEAN
                    if sea and d == 2:
                        continue        # as near as the open sea can be
                    e = d
                    for k in range(6):
                        cell = step(x, y, k)
                        if not cell:
                            continue
                        n = m.nib(*cell)
                        if not n:
                            continue
                        if not sea:
                            # Land and shallows join the area they touch.
                            if n == 1:
                                e = 1
                        elif n + 1 < FAR and (not e or n + 1 < e) and (
                                n == 1 or m.terrain(*cell) == OCEAN):
                            e = n + 1
                    if e != d:
                        m.set_nib(x, y, e)
                        changed = True
            back = not back
        # The open sea nearest the joined area next to an area not joined.
        best = 0
        for y in range(ROWS):
            for x in range(COLUMNS):
                d = m.nib(x, y)
                if not d or d == 1 or m.terrain(x, y) != OCEAN:
                    continue
                if best and d >= best:
                    continue
                for k in range(6):
                    cell = step(x, y, k)
                    if cell and not m.nib(*cell) \
                            and m.terrain(*cell) != OCEAN:
                        best, bx, by = d, x, y
                        break
        if not best:
            break
        # Back down the distances to the joined area, making shallows.
        x, y = bx, by
        while True:
            d = m.nib(x, y)
            m.set_terrain(x, y, SHALLOW_OCEAN)
            m.set_nib(x, y, 1)
            if d == 2:
                break
            for k in range(6):
                cell = step(x, y, k)
                if cell and m.nib(*cell) == d - 1 \
                        and m.terrain(*cell) == OCEAN:
                    x, y = cell
                    break
        joins += 1
    for y in range(ROWS):
        for x in range(COLUMNS):
            m.set_nib(x, y, 0)
    return joins


def resources(rng, m):
    """Step 7: a resource in the high nibble of the cells that have one. No
    two resources are neighbours, so a city's hexagons never hold more
    than a few. The cells are gone through in a fixed stride from a random
    start, which visits each once, since RES_STRIDE and CELLS share no
    factor."""
    count = 0

    def free(x, y):
        if m.nib(x, y):
            return False
        for k in range(6):
            cell = step(x, y, k)
            if cell and m.nib(*cell):
                return False
        return True

    placed = 0
    for _ in range(1000):
        if placed == TIN_NODES:
            break
        x = rng.rand(COLUMNS)
        y = rng.rand(2 * TIN_ROWS)
        if y >= TIN_ROWS:
            y += ROWS - 2 * TIN_ROWS
        if m.terrain(x, y) in (MOUNTAIN, HILLS, TUNDRA) and free(x, y):
            m.set_nib(x, y, TIN)
            placed += 1
    count = placed

    c = rng.rand(CELLS)
    for _ in range(CELLS):
        c += RES_STRIDE
        if c >= CELLS:
            c -= CELLS
        x, y = c % COLUMNS, c // COLUMNS
        chance, choices = RESOURCE_TERRAIN.get(m.terrain(x, y), (0, []))
        if count == MAX_RESOURCES or not choices:
            continue
        if rng.rand(100) >= chance or not free(x, y):
            continue
        pick = rng.rand(sum(w for _, w in choices))
        for r, w in choices:
            if pick < w:
                break
            pick -= w
        m.set_nib(x, y, r)
        count += 1


RES_STRIDE = 1597


def res_list(m):
    """The resources as the game keeps them (map_res_* in src/map.h), the
    nibbles cleared: hexagon row by row, column by column."""
    first, ax, kind = [], [], []
    for hy in range(2 * ROWS):
        first.append(len(ax))
        for x in range(hy & 1, COLUMNS, 2):
            y = (hy - (x & 1)) >> 1
            if m.nib(x, y):
                ax.append(x)
                kind.append(m.nib(x, y))
                m.set_nib(x, y, 0)
    first.append(len(ax))
    return first, ax, kind


def generate(seed, land_percent):
    """map_generate() in src/mapgen.c."""
    rng = Rng(seed)
    m = Map()
    make_land(rng, m, land_percent)
    fill_lakes(m)
    coast_distance(m)
    raise_mountains(rng, m)
    raise_hills(rng, m)
    climate(rng, m)
    erode(m)
    coast_distance(m)
    rivers(rng, m)
    clean_up(m)
    m.joins = join_seas(m)
    resources(rng, m)
    return m, res_list(m)


def write_dump(path, m, res):
    first, ax, kind = res
    with open(path, "wb") as f:
        f.write(bytes(v for row in m.t for v in row))
        f.write(bytes([len(ax)]) + bytes(first) + bytes(ax) + bytes(kind))


def write_png(path, m, res):
    """A rough preview: each hexagon a 6x8 brick, odd columns half a row down,
    which is the same arrangement the game draws. The map wraps east-west, so
    the left and right edges of the picture join."""
    cw, rh = 6, 4                       # per column, per hexagon row (hy)
    width, height = COLUMNS * cw, (2 * ROWS + 1) * rh
    pixels = [[(0, 0, 0)] * width for _ in range(height)]
    for y in range(ROWS):
        for x in range(COLUMNS):
            top = (2 * y + (x & 1)) * rh
            colour = COLOURS[m.terrain(x, y)]
            for py in range(top, top + 2 * rh):
                for px in range(x * cw, x * cw + cw):
                    pixels[py][px] = colour
    first, ax, kind = res
    for hy in range(2 * ROWS):
        for i in range(first[hy], first[hy + 1]):
            x, top = ax[i], hy * rh
            for py in range(top + rh - 1, top + rh + 1):
                for px in range(x * cw + 2, x * cw + 4):
                    pixels[py][px] = RESOURCE_COLOURS[kind[i]]
    raw = b"".join(b"\0" + bytes(c for p in row for c in p) for row in pixels)

    def chunk(kind, data):
        out = struct.pack(">I", len(data)) + kind + data
        return out + struct.pack(">I", zlib.crc32(kind + data) & 0xFFFFFFFF)

    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n")
        f.write(chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2,
                                           0, 0, 0)))
        f.write(chunk(b"IDAT", zlib.compress(raw, 9)))
        f.write(chunk(b"IEND", b""))


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("output", nargs="?")
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--land", type=int, default=35,
                        help="land as a percentage of the map (default 35)")
    parser.add_argument("--png", help="also write a preview image here")
    args = parser.parse_args()
    if not 5 <= args.land <= 70:
        sys.exit("mapgen: --land must be between 5 and 70")
    if not 0 <= args.seed <= 0xFFFF:
        sys.exit("mapgen: --seed is 16 bits")

    m, res = generate(args.seed, args.land)
    if args.output:
        write_dump(args.output, m, res)
    if args.png:
        write_png(args.png, m, res)
    counts = {}
    for r in res[2]:
        counts[r] = counts.get(r, 0) + 1
    names = ("", "grain", "fish", "cattle", "olives", "wine", "cedar",
             "copper", "tin", "dye", "horses")
    print("mapgen: seed %d, resources " % args.seed + ", ".join(
        "%s %d" % (names[r], counts.get(r, 0)) for r in range(1, HORSES + 1)))


if __name__ == "__main__":
    main()
