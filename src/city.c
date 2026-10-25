#include "city.h"
#include "culture.h"
#include "growth.h"
#include "civ.h"
#include "map.h"
#include "minimap.h"
#include "sheet.h"
#include "unit.h"
#include "road.h"

byte city_count;
byte city_founded;
// In HIGH_BSS, out of the 32 KB; city_count says which entries mean
// anything, and city_reset() sets it for every game.
HIGH_BSS byte city_owner[MAX_CITIES];
HIGH_BSS byte city_ax[MAX_CITIES];
HIGH_BSS byte city_ay[MAX_CITIES];
HIGH_BSS byte city_founder[MAX_CITIES];
HIGH_BSS byte city_name[MAX_CITIES];
HIGH_BSS byte city_build[MAX_CITIES];
HIGH_BSS word city_shields[MAX_CITIES];
HIGH_BSS byte city_size[MAX_CITIES];
HIGH_BSS byte city_food[MAX_CITIES];
HIGH_BSS byte city_worked[CITY_WORKED_BYTES][MAX_CITIES];
HIGH_BSS byte city_culture[MAX_CITIES];

// How many cities each player has named, so the next gets the next name.
byte city_named[PLAYERS];

void city_reset(void)
{
    byte p;
    city_count = 0;
    city_founded = 0;
    for (p = 0; p < PLAYERS; ++p) city_named[p] = 0;
}

byte city_at(byte ax, byte ay)
{
    byte c;
    for (c = 0; c < city_count; ++c)
        if (city_ax[c] == ax && city_ay[c] == ay) return c;
    return CITY_NONE;
}

byte city_coastal(byte c)
{
    byte dir, nx, ny;
    for (dir = 0; dir < 6; ++dir)
        if (map_neighbour(city_ax[c], city_ay[c], dir, &nx, &ny)
            && MAP_TERRAIN(nx, ny) == TILE_SHALLOW_OCEAN)
            return 1;
    return 0;
}

byte culture_holder(byte bit)
{
    byte c;
    for (c = 0; c < city_count; ++c)
        if (city_culture[c] & bit) return city_owner[c];
    return CULTURE_NONE;
}

byte city_found(byte owner, byte ax, byte ay)
{
    byte c = city_count;
    if (c == MAX_CITIES || city_at(ax, ay) != CITY_NONE) return CITY_NONE;
    city_founder[c] = owner;
    city_ax[c] = ax;
    city_ay[c] = ay;
    city_name[c] = city_named[owner]++;
    city_size[c] = 1;
    city_food[c] = 0;
    city_culture[c] = 0;
    city_founded |= (byte)(1 << owner);
    ++city_count;
    city_capture(c, owner);
    unit_mark(ax, ay);
    road_add(ax, ay);               // every city stands on a road
    return c;
}

// **A city of the human's sees its whole reach**, the RING it works
// (map.h), from the day it is founded or taken: the six neighbours'
// neighbours are the rest of it.
void city_capture(byte c, byte owner)
{
    byte i, nx, ny;

    if (owner == PLAYER_HUMAN)
        for (i = 0; i < RING_INNER; ++i)
            if (map_neighbour(city_ax[c], city_ay[c], i, &nx, &ny))
                map_explore(nx, ny);
    city_owner[c] = owner;
    city_worked[0][c] = city_worked[1][c] = city_worked[2][c] = 0;
    city_build[c] = KIND_WARRIOR;
    city_shields[c] = 0;
    minimap_plot(city_ax[c], city_ay[c]);
}

void city_destroy(byte c)
{
    byte ax = city_ax[c], ay = city_ay[c];
    byte last = --city_count;

    city_owner[c] = city_owner[last];
    city_founder[c] = city_founder[last];
    city_ax[c] = city_ax[last];
    city_ay[c] = city_ay[last];
    city_name[c] = city_name[last];
    city_build[c] = city_build[last];
    city_shields[c] = city_shields[last];
    city_size[c] = city_size[last];
    city_food[c] = city_food[last];
    city_worked[0][c] = city_worked[0][last];
    city_worked[1][c] = city_worked[1][last];
    city_worked[2][c] = city_worked[2][last];
    city_culture[c] = city_culture[last];
    unit_mark(ax, ay);
}

const char *city_name_of(byte c)
{
    // Past the end of a list, names repeat; that is a problem for when a
    // player has founded a dozen cities, which nobody can yet.
    return civ_city_name(player_civ[city_founder[c]], city_name[c]);
}

byte city_production(byte c)
{
    city_work(c);
    return work_shields;
}

// Tenths, by game_level (civ.h).
static const byte level_tenths[LEVELS] = { 15, 12, 10, 8, 7 };

word level_scale(byte owner, word v)
{
    if (owner < PLAYER_FIRST_AI || owner >= PLAYERS) return v;
    return v * level_tenths[game_level] / 10;
}

// The people of owner, or CIVS for the barbarians.
static byte people(byte owner)
{
    return owner < PLAYERS ? player_civ[owner] : CIVS;
}

word kind_cost(byte owner, byte kind)
{
    word cost = level_scale(owner, unit_kind_cost[kind]);
    byte civ = people(owner);
    if ((civ == CIV_HITTITES && kind == KIND_CHARIOT)
        || (civ == CIV_MYCENAEANS && KIND_IS_SHIP(kind) && kind < KINDS))
        cost -= cost >> 2;
    return cost;
}

word city_cost(byte c)
{
    return kind_cost(city_owner[c], city_build[c]);
}

word city_box(byte c)
{
    word box = level_scale(city_owner[c], GROWTH_BOX(city_size[c]));
    if (people(city_owner[c]) == CIV_EGYPTIANS) box -= box >> 2;
    return box > CITY_BOX_MOST ? CITY_BOX_MOST : box;
}

byte city_turns_left(byte c)
{
    word cost = city_cost(c);
    word have = city_shields[c];
    byte rate = city_production(c);

    if (have >= cost) return 1;
    return (byte)((cost - have + rate - 1) / rate);
}

byte city_produce(byte c)
{
    byte kind = city_build[c];
    word cost = city_cost(c);

    if (kind == KIND_NONE) return KIND_NONE;    // told to build nothing
    city_shields[c] += city_production(c);
    if (city_shields[c] < cost) return KIND_NONE;
    if (KIND_IS_BUILDING(kind)) {               // src/culture.h
        city_shields[c] -= cost;
        culture_built(c, kind);
        return kind;
    }
    if ((kind == KIND_SETTLER && city_size[c] < 2)
        || unit_create(kind, city_owner[c], city_ax[c], city_ay[c])
               == UNIT_NONE) {
        city_shields[c] = cost;
        return KIND_NONE;
    }
    city_shields[c] -= cost;
    if (kind == KIND_SETTLER) {
        // The citizen goes with it, and the food box is the smaller one.
        --city_size[c];
        if (city_food[c] >= city_box(c)) city_food[c] = (byte)(city_box(c) - 1);
    }
    return kind;
}
