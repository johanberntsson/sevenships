// The orders keys of the unit with the orders, and the Orders menu that
// offers them, out of game.c (25 Oct 2026, to make resident room for
// roads), and a step of the unit, or an attack (for the wonders): in
// region 2, beside the improvements they start, called through the stubs
// of game_order(), game_orders_menu() and game_move() (gameint.h); and a
// click on the map, which moves the unit (26 Oct 2026, game_click_hex()).
#define game_order game_order_r2
#define game_orders_menu game_orders_menu_r2
#define game_move game_move_r2
#define game_click_hex game_click_hex_r2

#include "gameint.h"
#include "boat.h"
#include "civ.h"
#include "combat.h"
#include "diplomacy.h"
#include "popup.h"
#include "city.h"
#include "cityscreen.h"
#include "goto.h"
#include "emporium.h"
#include "improve.h"
#include "map.h"
#include "menu.h"
#include "unit.h"

#pragma clang section text="r2text" rodata="r2rodata"

// The active unit u attacks (nx, ny) -- after a declaration of war, if it
// takes one. How it went is a popup, as for every fight of the human's
// (fight.h), shown by game_run().
static void attack(byte u, byte nx, byte ny)
{
    byte kind = unit_kind[u];
    byte o = combat_holder(nx, ny);

    if (!unit_kind_attack[kind]) {
        message(unit_kind_name[kind], "cannot attack");
        return;
    }
    if (!at_war(PLAYER_HUMAN, o)) {
        popup_begin();
        popup_add("Declare war on the ");
        popup_add(player_people(o));
        popup_add("?");
        if (!popup_ask_shown()) return;
        declare_war(PLAYER_HUMAN, o);
    }
    message("", "");
    combat_attack(u, nx, ny);
    dirty = 1;
    hover_dirty = 1;
    select_unit(u);
}

void game_move(byte dir)
{
    byte u = unit_active;
    byte nx, ny, t;

    if (!map_neighbour(unit_ax[u], unit_ay[u], dir, &nx, &ny)) return;
    t = unit_can_enter(PLAYER_HUMAN, unit_kind[u], nx, ny);
    if (t == ENTER_WATER) {
        message("Cannot swim", "");
        return;
    }
    if (t == ENTER_LAND) {
        message("Ships keep", "to the shallows");
        return;
    }
    if (t == ENTER_MOUNTAIN) {
        message("Chariots keep", "off mountains");
        return;
    }
    if (t == ENTER_FOREIGN) {
        attack(u, nx, ny);
        return;
    }
    message("", "");
    unit_move_to(u, nx, ny);
    if (!unit_moves[u]) unit_wake_cargo(u);
    dirty = 1;
    contact = 1;
    hover_on(nx, ny);
    if (unit_moves[u]) {
        if (!in_view(nx, ny)) centre_on(nx, ny);
        draw_orders();
    } else {
        select_unit(u);
    }
}

static void found_city(void)
{
    byte u = unit_active;
    byte c;

    if (unit_kind[u] == KIND_MERCHANT) {
        emporium_order(u);
        return;
    }
    if (unit_kind[u] != KIND_SETTLER) {
        message("Only settlers", "found cities");
        return;
    }
    c = city_found(PLAYER_HUMAN, unit_ax[u], unit_ay[u]);
    if (c == CITY_NONE) {
        message("Cannot found", "a city here");
        return;
    }
    unit_remove(u);
    message("Founded", city_name_of(c));
    dirty = 1;
    hover_dirty = 1;
    select_unit(u);
}

// The active unit, a builder, starts irrigating, mining or building a
// pasture or a road where it stands.
static void improve(byte orders)
{
    byte u = unit_active;
    const char *what = orders == ORDERS_IRRIGATE ? "irrigate"
                       : orders == ORDERS_MINE ? "mine"
                       : orders == ORDERS_ROAD ? "build roads"
                       : "build pastures";

    if (unit_kind[u] != KIND_BUILDER) {
        message("Only builders", what);
        return;
    }
    if (!improve_can(orders, unit_ax[u], unit_ay[u])) {
        if (orders == ORDERS_PASTURE) {
            // It fails only where there is one, or no cattle or horses.
            if (RES_PASTURE(map_resource_at(unit_ax[u], unit_ay[u])))
                message("A pasture", "already");
            else
                message("Pastures are for", "cattle and horses");
        } else {
            line_clear();
            line_add("Cannot ");
            line_add(what);
            message(line, "here");
        }
        return;
    }
    improve_start(u, orders);
    select_unit(u);
}

// The active unit is disbanded, once the player says so, as with Shift+D
// in Civilization -- a ship at sea with those aboard it, who would
// otherwise be left on the water.
static void disband(void)
{
    byte u = unit_active, kind = unit_kind[u], v;
    byte ax = unit_ax[u], ay = unit_ay[u];

    popup_begin();
    popup_add("Disband this ");
    popup_add(unit_kind_name[kind]);
    popup_add("?");
    if (!popup_ask_shown()) return;
    // A ship is at sea anywhere but in a city.
    if (KIND_IS_SHIP(kind) && city_at(ax, ay) == CITY_NONE)
        for (v = 0; v < MAX_UNITS; ++v)
            if (unit_kind[v] != KIND_NONE && !KIND_IS_SHIP(unit_kind[v])
                && unit_ax[v] == ax && unit_ay[v] == ay
                && unit_owner[v] == PLAYER_HUMAN)
                unit_remove(v);
    unit_remove(u);
    message(unit_kind_name[kind], "disbanded");
    dirty = 1;
    hover_dirty = 1;
    select_unit(u);
}

void game_order(byte key)
{
    byte u = unit_active;

    switch (key) {
    case 'b': found_city(); break;
    case 'f':
        unit_orders[u] = ORDERS_FORTIFY;
        select_unit(u);
        break;
    case 'i':
        // A fishing boat works the fish it is on.
        if (unit_kind[u] == KIND_BOAT) boat_order(u);
        else improve(ORDERS_IRRIGATE);
        break;
    case 'm': improve(ORDERS_MINE); break;
    case 'p': improve(ORDERS_PASTURE); break;
    case 'r': improve(ORDERS_ROAD); break;
    case 'd': disband(); break;
    }
}

void game_orders_menu(void)
{
    byte command = MENU_NONE, second = MENU_NONE, road = MENU_NONE;
    if (unit_active != UNIT_NONE) {
        byte kind = unit_kind[unit_active];
        if (kind == KIND_SETTLER) command = MENU_FOUND_CITY;
        else if (kind == KIND_WARRIOR || kind == KIND_SPEARMAN
                 || kind == KIND_CHARIOT)
            command = MENU_FORTIFY;
        else if (kind == KIND_BUILDER) {
            // A pasture instead of a mine on cattle and horses.
            command = MENU_IRRIGATE;
            second = RES_PASTURE(map_resource_at(unit_ax[unit_active],
                                                 unit_ay[unit_active]))
                     ? MENU_PASTURE : MENU_MINE;
            road = MENU_ROAD;
        } else if (kind == KIND_BOAT) {
            command = MENU_FISH;
        } else if (kind == KIND_MERCHANT) {
            command = MENU_EMPORIUM;
        } else if (kind == KIND_GALLEY) {
            command = MENU_ESCORT;              // with a merchant ship (unit.h)
        }
    }
    // Every unit can be sent somewhere.
    menu_orders(command, second, road,
                unit_active != UNIT_NONE ? MENU_GOTO : MENU_NONE);
}

static void open_city(byte c)
{
    city_screen(c);
    // Its unit list may have woken a unit, with the turn otherwise over,
    // or fortified the one that had the orders.
    if (unit_active == UNIT_NONE || unit_orders[unit_active] != ORDERS_NONE)
        select_unit(unit_active);
    draw_screen();
}

// A click on hexagon (ax, ay) of the map, by the arrow or by Return in tile
// select. After g, it is where the unit is to go. Next to the unit with
// orders it moves the unit there; on one of
// the player's cities it opens the city screen, and on one next to the unit
// with orders it asks which; on one of the player's own
// units that still has moves it gives that unit the orders, waking it if it
// was fortified. Anywhere else it centres the view there.
void game_click_hex(byte ax, byte ay)
{
    byte u, dir = DIR_NONE, c;

    // The active unit is to go there, and sets off (goto.h).
    if (going) {
        select_end();
        if (unit_active != UNIT_NONE) goto_order(unit_active, ax, ay);
        return;
    }

    c = city_at(ax, ay);
    if (c != CITY_NONE && city_owner[c] != PLAYER_HUMAN) c = CITY_NONE;
    if (unit_active != UNIT_NONE) {
        dir = map_direction(unit_ax[unit_active], unit_ay[unit_active],
                            ax, ay);
        // Next to the unit and one of the player's cities: ask which.
        if (dir != DIR_NONE
            && (c == CITY_NONE
                || popup_choose(city_name_of(c), POPUP_MOVE_VIEW))) {
            game_move(dir);
            return;
        }
    }
    if (c != CITY_NONE) {
        open_city(c);
        return;
    }
    if (unit_active != UNIT_NONE
        && unit_ax[unit_active] == ax && unit_ay[unit_active] == ay) return;
    for (u = 0; u < MAX_UNITS; ++u) {
        if (unit_kind[u] != KIND_NONE && unit_owner[u] == PLAYER_HUMAN
            && unit_ax[u] == ax && unit_ay[u] == ay && unit_moves[u]) {
            unit_orders[u] = ORDERS_NONE;
            unit_active = u;
            dirty = 1;
            hover_on(ax, ay);
            draw_orders();
            return;
        }
    }
    centre_on(ax, ay);
}
