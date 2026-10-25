// Resident: road_at() is needed by moves and by everything that reads a
// yield, from every region. road_add() is in region 2, in src/roadadd.c.
#include "road.h"

HIGH_BSS byte road_count;
HIGH_BSS byte road_first[2 * MAP_ROWS + 1];
HIGH_BSS byte road_ax[MAX_ROADS];
HIGH_BSS byte road_mask[MAX_ROADS];

void road_reset(void)
{
    byte i;
    road_count = 0;
    for (i = 0; i < sizeof road_first; ++i) road_first[i] = 0;
}

byte road_at(byte ax, byte ay)
{
    byte hy = (byte)(2 * ay + (ax & 1));
    byte i, n = road_first[hy + 1];
    for (i = road_first[hy]; i < n; ++i)
        if (road_ax[i] == ax) return i;
    return ROAD_NONE;
}
