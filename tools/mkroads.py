#!/usr/bin/env python3
"""Draw the road tiles: tiles/road_e.png, road_se.png, road_sw.png and
road_dot.png.

**A road is drawn as joins between junctions** (src/road.h): the
hexagon with a road to its east, south-east or south-west draws the whole
join from its junction to that neighbour's, one tile and one call of
view_draw_tile() each, which is what roads cost the redraw -- half-roads,
one each way, cost twice the calls. road_dot.png is a road leading
nowhere. A join is wider than a hexagon, so its tile is placed away from
the hexagon's own (view.c): the east one 4 characters (16 pixels) to the
right, the others a character row (8 lines) down, and the south-west one 4
characters to the left as well. They are 32x32, the sheet's tile.

**The junction is at line 10** of the hexagon, above its middle, so that
the east join is inside one character row, one DMA job; the diagonals take
four. The road runs 5 lines above the middle of every hexagon, so nothing
looks out of line. The colours are alltiles' road_*.png lightened: its
roads join two edges each, so would need a tile for every pair, and its
colours and width made a dark lattice once the hexagons round a city all
had roads. A manual step, like `make` in tiles/: run it and check the PNGs
in.
"""
import math
from PIL import Image

# Junction to junction, in each tile's own pixels: the hexagon's junction is
# at (15.5, 10) of its own tile, and the neighbours' 32 across, or 16 across
# and 24 down.
JOINS = {
    "e": ((-0.5, 10.0), (31.5, 10.0)),      # tile at +16, 0
    "se": ((15.5, 2.0), (31.5, 26.0)),      # tile at 0, +8
    "sw": ((31.5, 2.0), (15.5, 26.0)),      # tile at -16, +8
    "dot": ((15.5, 10.0), (15.5, 10.0)),
}
ROAD = (150, 124, 88, 255)
EDGE = (104, 86, 62, 255)
HALF_WIDTH = 1.6        # pixels either side of the middle of the road
BORDER = 0.7            # of which the darker edge


def distance_to(px, py, a, b):
    ax, ay = a
    bx, by = b
    dx, dy = bx - ax, by - ay
    t = ((px - ax) * dx + (py - ay) * dy) / (dx * dx + dy * dy or 1)
    t = max(0.0, min(1.0, t))
    return math.hypot(px - (ax + t * dx), py - (ay + t * dy))


for name, (a, b) in JOINS.items():
    im = Image.new("RGBA", (32, 32), (0, 0, 0, 0))
    for y in range(32):
        for x in range(32):
            d = distance_to(x, y, a, b)
            if d <= HALF_WIDTH:
                im.putpixel((x, y), ROAD if d <= HALF_WIDTH - BORDER else EDGE)
    im.save("tiles/road_%s.png" % name)
