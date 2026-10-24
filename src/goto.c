// In region 4 (src/region.h): called through the stubs in src/dispatch.s,
// under these names here.
#define goto_order goto_order_r4
#define goto_turn goto_turn_r4

#include "goto.h"
#include "city.h"
#include "gameint.h"
#include "map.h"
#include "path.h"
#include "sheet.h"
#include "unit.h"

#pragma clang section text="r4text" rodata="r4rodata"

// Unit u, on its way, takes the steps its moves allow. Arrived, or with
// the way blocked, it has its orders back; a galley stopping wakes those
// aboard next to land, as after the human's own moves.
static void walk(byte u)
{
    byte kind = unit_kind[u], tx = unit_goto_x[u], ty = unit_goto_y[u];
    byte dir, nx, ny, stop = 0;

    while (unit_moves[u] && (unit_ax[u] != tx || unit_ay[u] != ty)) {
        dir = map_direction(unit_ax[u], unit_ay[u], tx, ty);
        if (dir == DIR_NONE)
            dir = path_route(unit_ax[u], unit_ay[u], tx, ty, kind, 1);
        if (dir == DIR_NONE
            || !map_neighbour(unit_ax[u], unit_ay[u], dir, &nx, &ny)
            || unit_can_enter(PLAYER_HUMAN, kind, nx, ny) != ENTER_OK) {
            message(unit_kind_name[kind], "cannot get through");
            stop = 1;
            break;
        }
        unit_move_to(u, nx, ny);
        --unit_moves[u];
    }
    if (unit_ax[u] == tx && unit_ay[u] == ty) stop = 1;
    if (stop) unit_orders[u] = ORDERS_NONE;
    if (stop || !unit_moves[u]) unit_wake_cargo(u);
}

void goto_order(byte u, byte ax, byte ay)
{
    byte kind = unit_kind[u], t = MAP_TERRAIN(ax, ay), c = city_at(ax, ay);
    byte fx = unit_ax[u], fy = unit_ay[u];

    if (ax == fx && ay == fy) return;
    if (!MAP_IS_EXPLORED(ax, ay)) {
        message("Unexplored", "");
        return;
    }
    // A ship to the shallows or one of the human's own cities, anything
    // else to land; a chariot never to mountains.
    if (KIND_IS_SHIP(kind)
        ? t != TILE_SHALLOW_OCEAN
          && (c == CITY_NONE || city_owner[c] != PLAYER_HUMAN)
        : t == TILE_OCEAN || t == TILE_SHALLOW_OCEAN
          || (kind == KIND_CHARIOT && t == TILE_MOUNTAIN)) {
        message("Cannot go there", "");
        return;
    }
    if (map_direction(fx, fy, ax, ay) == DIR_NONE
        && path_route(fx, fy, ax, ay, kind, 1) == DIR_NONE) {
        message("No way there", "");
        return;
    }
    message("", "");
    unit_goto_x[u] = ax;
    unit_goto_y[u] = ay;
    unit_orders[u] = ORDERS_GOTO;
    walk(u);
    dirty = 1;
    contact = 1;
    // On to the next unit, unless it has arrived with moves to spare:
    // then u again, which comes after u - 1 (after UNIT_NONE for u 0).
    select_unit(unit_orders[u] == ORDERS_NONE && unit_moves[u]
                ? (byte)(u - 1) : u);
}

void goto_turn(void)
{
    byte u;

    for (u = 0; u < MAX_UNITS; ++u)
        if (unit_kind[u] != KIND_NONE && unit_owner[u] == PLAYER_HUMAN
            && unit_orders[u] == ORDERS_GOTO)
            walk(u);
}
