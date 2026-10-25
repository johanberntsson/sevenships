// In region 2 (src/region.h), with the builders: called through the stub
// in src/dispatch.s, under this name here.
#define road_add road_add_r2

#include "road.h"

#pragma clang section text="r2text" rodata="r2rodata"

// For each DIR_*, the bit of the opposite direction, which is the way back
// from that neighbour: E and W, SE and NW, SW and NE.
static const byte back[6] = { 1 << 3, 1 << 4, 1 << 5, 1 << 0, 1 << 1, 1 << 2 };

void road_add(byte ax, byte ay)
{
    byte hy = (byte)(2 * ay + (ax & 1));
    byte i, j, dir, nx, ny, m = 0;

    if (road_count == MAX_ROADS || road_at(ax, ay) != ROAD_NONE) return;
    // In column order within the row: everything after it moves up one.
    for (i = road_first[hy]; i < road_first[hy + 1] && road_ax[i] < ax; ++i)
        ;
    for (j = road_count; j > i; --j) {
        road_ax[j] = road_ax[j - 1];
        road_mask[j] = road_mask[j - 1];
    }
    road_ax[i] = ax;
    ++road_count;
    for (j = hy + 1; j < sizeof road_first; ++j) ++road_first[j];
    // Only now, with the rows after it moved up, can road_at() be asked:
    // each neighbour's road leads here, and this one there. **One loop,
    // not two**: with the mask worked out in a loop of its own first,
    // Calypsi 5.18 started this one's direction at the mask, which it took
    // to be the 0 still in A, so no road ever learned of a newer one.
    for (dir = 0; dir < 6; ++dir) {
        if (!map_neighbour(ax, ay, dir, &nx, &ny)) continue;
        j = road_at(nx, ny);
        if (j == ROAD_NONE) continue;
        m |= (byte)(1 << dir);
        road_mask[j] |= back[dir];
    }
    road_mask[i] = m;
}
