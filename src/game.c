// The turn loop and the player's side of it.
//
// **Everything runs off one loop that goes round once a frame**, in
// game_run(): wait for the beam to leave the map window, redraw the view if
// anything changed, then read one frame's input and act on it. There is no
// nested "wait for a key" anywhere, because the mouse and the joystick have
// to be read every frame whatever the game is waiting for. The state that
// says what it is waiting for is just unit_active: a unit wanting orders, or
// UNIT_NONE and the end-of-turn prompt in the panel.

#include "game.h"
#include "gameint.h"
#include "growth.h"
#include "help.h"
#include "ai.h"
#include "treasury.h"
#include "city.h"
#include "cityscreen.h"
#include "topcities.h"
#include "save.h"
#include "score.h"
#include "text.h"
#include "civ.h"
#include "diplomacy.h"
#include "combat.h"
#include "fight.h"
#include "goto.h"
#include "sevenships.h"
#include "input.h"
#include "map.h"
#include "menu.h"
#include "minimap.h"
#include "popup.h"
#include "screen.h"
#include "sheet.h"
#include "unit.h"
#include "view.h"

#define PANEL_WIDTH VIEW_LEFT

#define TEXT_COLOUR COLOR_LIGHTGREEN

// Where the view sits on the map, in screen characters; see view_redraw().
word off_x;
word off_y;

byte dirty;              // the view wants redrawing
byte contact;            // see whether a people has come in sight
byte greet;              // peoples just met, greeted after a redraw
byte hover_dirty;        // the readout of what is under the arrow
byte hover_unit;         // ... or on hover_ax/ay; see hover.c
byte hover_ax, hover_ay;
byte game_over;
byte quit;
byte retired;            // quit by Game/Retire or Load Game: game_run()'s result
byte loaded;             // from a saved game: no opening popup
byte game_end_wait;      // see game.h
static byte end_due;     // the last unit has had its orders: end the turn
static byte fight_shown;  // fights shown of fight_count (combat.h)
static byte fight_placed; // ... and the next brought into view
word game_turn;
word game_year;

// **Tile select**, Civilization's keyboard stand-in for the mouse: Tab puts
// a ring on a hexagon, the cursor keys (or the unit's moving keys) move it,
// Return does what a click on it would, and Tab or Esc puts it away. While
// it is up the ring blinks rather than the active unit.
byte selecting;
byte sel_ax, sel_ay;
static byte sel_hidden;         // the ring's half of the blink
byte going;                     // ... put up by g: the ring picks a GOTO

// The active unit blinks: BLINK_FRAMES drawn, BLINK_FRAMES not, half a
// second each at 50 Hz. Each change is a redraw of the unit's hexagon row
// (view_redraw()'s band), not of one hexagon: a unit's tile overlaps five
// neighbours, so putting the terrain back under it means redrawing parts
// of all of them in painter's order -- and until 26 Oct 2026 it was the
// whole view, which on a busy screen ran into the beam on a real MEGA65.
#define BLINK_FRAMES 25
static byte blink;

// ------------------------------------------------------------------- text

// A line of text built up a piece at a time, so that no printf is linked.
char line[41];
byte line_len;

void line_clear(void)
{
    line_len = 0;
    line[0] = 0;
}

void line_add(const char *s)
{
    while (*s && line_len < sizeof line - 1) line[line_len++] = *s++;
    line[line_len] = 0;
}

// "Hittite Warrior".
void line_add_unit(byte owner, byte kind)
{
    line_add(player_adjective(owner));
    line_add(" ");
    line_add(unit_kind_name[kind]);
}

void line_add_num(word v)
{
    char digits[6];
    byte i = sizeof digits - 1;
    digits[i] = 0;
    do { digits[--i] = (char)('0' + v % 10); v /= 10; } while (v);
    line_add(digits + i);
}

// Text in the panel down the left, padded with spaces to the panel's width
// so that it overwrites whatever was there.
static void panel_text(byte row, const char *s, byte colour)
{
    byte x = 0;
    while (*s && x < PANEL_WIDTH) screen_putcxy(x++, row, *s++, colour);
    while (x < PANEL_WIDTH) screen_putcxy(x++, row, ' ', colour);
}

// Below the minimap and its frame, which take rows 1-7.
#define ROW_CIV 9
#define ROW_YEAR 10
#define ROW_TURN 11
#define ROW_GOLD 12
#define ROW_UNIT 13
#define ROW_MOVES 14
#define ROW_PROMPT 16           // two rows
#define ROW_MESSAGE 19          // two rows

// Two lines of message in the panel. `second` may be `line`.
void message(const char *first, const char *second)
{
    panel_text(ROW_MESSAGE, first, COLOR_YELLOW);
    panel_text(ROW_MESSAGE + 1, second, COLOR_YELLOW);
}

static void draw_frame(void)
{
    menu_draw();
    panel_text(ROW_CIV, civ_adjective(player_civ[PLAYER_HUMAN]), TEXT_COLOUR);
    // Wider than the window, so it ends where the window ends rather than
    // starting where it starts.
    if (input_real_hw)
        screen_putsxy(VIEW_LEFT + VIEW_COLUMNS - 17, SCREEN_ROWS - 1,
                      "HELP for the keys", TEXT_COLOUR);
    else
        screen_putsxy(VIEW_LEFT + VIEW_COLUMNS - 17, SCREEN_ROWS - 1,
                      "PgUp for the keys", TEXT_COLOUR);
}

void draw_status(void)
{
    line_clear();
    line_add_num(game_year);
    line_add(" BC");
    panel_text(ROW_YEAR, line, TEXT_COLOUR);
    line_clear();
    line_add("Turn ");
    line_add_num(game_turn);
    panel_text(ROW_TURN, line, TEXT_COLOUR);
    line_clear();
    line_add_num(player_gold[PLAYER_HUMAN]);
    line_add(" gold");
    panel_text(ROW_GOLD, line, TEXT_COLOUR);
}

// The unit waiting for orders, or the end-of-turn prompt if there is none.
void draw_orders(void)
{
    byte u = unit_active;
    byte r, v;

    if (u != UNIT_NONE) {
        panel_text(ROW_UNIT, unit_kind_name[unit_kind[u]], COLOR_WHITE);
        line_clear();
        line_add("Moves: ");
        // In thirds since roads: "Moves: 1 1/3", "Moves: 2/3".
        v = unit_moves[u];
        if (v >= MOVE_FULL || !v) line_add_num(v / MOVE_FULL);
        v %= MOVE_FULL;
        if (v) {
            if (unit_moves[u] >= MOVE_FULL) line_add(" ");
            line_add_num(v);
            line_add("/3");
        }
        panel_text(ROW_MOVES, line, TEXT_COLOUR);
        for (r = 0; r < 2; ++r) panel_text(ROW_PROMPT + r, "", TEXT_COLOUR);
        return;
    }
    panel_text(ROW_UNIT, "", TEXT_COLOUR);
    panel_text(ROW_MOVES, "", TEXT_COLOUR);
    if (game_over) {
        panel_text(ROW_PROMPT, "Game over", COLOR_WHITE);
        panel_text(ROW_PROMPT + 1, "Press a key", TEXT_COLOUR);
    } else {
        panel_text(ROW_PROMPT, "End of turn", COLOR_WHITE);
        panel_text(ROW_PROMPT + 1, "Press Return", ATTR_BLINK | TEXT_COLOUR);
    }
}

// ---------------------------------------------------------------- the view

// Which hexagon is under the tip of the arrow. 0 if the arrow is not over
// the map window. A sprite unit is two H640 pixels, so a character is four
// units across and eight down.
byte pointer_hex(byte *ax, byte *ay)
{
    byte col = (byte)(input_x >> 2);
    byte row = (byte)(input_y >> 3);
    word cx;

    if (col < VIEW_LEFT || col >= VIEW_LEFT + VIEW_COLUMNS) return 0;
    if (row < VIEW_TOP || row >= VIEW_TOP + VIEW_ROWS) return 0;
    cx = off_x + (col - VIEW_LEFT);
    if (cx >= MAP_WIDTH_CHARS) cx -= MAP_WIDTH_CHARS;
    map_cell_at(cx, off_y + (row - VIEW_TOP), ax, ay);
    return 1;
}

// Whether all of (ax, ay)'s tile is inside the window.
byte in_view(byte ax, byte ay)
{
    word x = MAP_CELL_X(ax) + MAP_WIDTH_CHARS - off_x;
    int y = (int)MAP_CELL_Y(ax, ay) - (int)off_y;
    if (x >= MAP_WIDTH_CHARS) x -= MAP_WIDTH_CHARS;
    return x <= VIEW_COLUMNS - TILE_EXPANDED_COLUMNS
           && y >= 0 && y <= VIEW_ROWS - TILE_ROWS;
}

void centre_on(byte ax, byte ay)
{
    int x = (int)MAP_CELL_X(ax) + TILE_EXPANDED_COLUMNS / 2 - VIEW_COLUMNS / 2;
    int y = (int)MAP_CELL_Y(ax, ay) + TILE_ROWS / 2 - VIEW_ROWS / 2;
    if (x < 0) x += MAP_WIDTH_CHARS;
    if (y < VIEW_MIN_OFF_Y) y = VIEW_MIN_OFF_Y;
    if (y > VIEW_MAX_OFF_Y) y = VIEW_MAX_OFF_Y;
    off_x = (word)x;
    off_y = (word)y;
    dirty = 1;
}

// (ax, ay) brought into view above the middle of the window, which a popup
// is about to cover -- or below it, near the bottom of the map, where the
// view cannot go far enough down. Row 1, so that whichever of the
// player's is next to it is in view too if it is below, as it is half the
// time: for a people just met, and for a fight.
#define MET_ROW 1               // the window row of the tile's top
#define MET_ROW_BELOW 17
static void show_at(byte ax, byte ay)
{
    int y = (int)MAP_CELL_Y(ax, ay) - MET_ROW;

    centre_on(ax, ay);
    if (y > VIEW_MAX_OFF_Y) y -= MET_ROW_BELOW - MET_ROW;
    if (y < VIEW_MIN_OFF_Y) y = VIEW_MIN_OFF_Y;
    if (y > VIEW_MAX_OFF_Y) y = VIEW_MAX_OFF_Y;
    off_y = (word)y;
}

void show_met(void)
{
    show_at(diplomacy_ax, diplomacy_ay);
}

// ------------------------------------------------------------------- turns

// The readout on (ax, ay) until the arrow moves.
void hover_on(byte ax, byte ay)
{
    hover_ax = ax;
    hover_ay = ay;
    hover_unit = 1;
    hover_dirty = 1;
}

// Tile select's ring onto unit u, if it is up: when the orders pass to a
// unit, and on c, the ring goes with them.
static void select_follow(byte u)
{
    if (!selecting) return;
    sel_ax = unit_ax[u];
    sel_ay = unit_ay[u];
    dirty = 1;
    hover_dirty = 1;
}

// Hand the orders to the next unit after `after` that wants some, bringing
// it into view. With none left, the panel asks for the end of the turn.
void select_unit(byte after)
{
    byte was = unit_active;
    byte u = unit_next(PLAYER_HUMAN, after);

    if (game_over || !player_alive(PLAYER_HUMAN)) {
        game_over = 1;
        u = UNIT_NONE;
    }
    unit_active = u;
    // Orders to the last unit end the turn, unless End of Turn is on; a
    // turn with nobody to give orders to still waits for Return.
    end_due = was != UNIT_NONE && u == UNIT_NONE;
    if (was != u) dirty = 1;   // another unit blinks
    if (u != UNIT_NONE) {
        hover_on(unit_ax[u], unit_ay[u]);
        select_follow(u);
    }
    if (u != UNIT_NONE && !in_view(unit_ax[u], unit_ay[u]))
        centre_on(unit_ax[u], unit_ay[u]);
    draw_orders();
}



// --------------------------------------------------------------- orders

// The keypad in xemu, as res/xemu-keymap.cfg delivers it: 7 9 4 6 1 3.
static const byte pad_dir[] = { DIR_NW, DIR_NE, DIR_W, DIR_E, DIR_SW, DIR_SE };

// The direction a moving key stands for, or DIR_NONE.
byte key_dir(byte key)
{
    if (!input_real_hw && key >= KEY_PAD_FIRST && key <= KEY_PAD_LAST)
        return pad_dir[(byte)(key - KEY_PAD_FIRST)];

    // The moving keys of a real MEGA65, 0 - p * : =, went on 24 Oct 2026
    // (the user's: they were awkward, and tile select moves a unit from
    // the keyboard), and p builds a pasture now.
    return DIR_NONE;
}

static void select_begin(void);

void unit_key(byte key)
{
    byte u = unit_active;
    byte dir = key_dir(key);

    if (dir != DIR_NONE) {
        game_move(dir);
        return;
    }

    switch (key) {
    case KEY_SPACE:                         // no orders this turn
        unit_moves[u] = 0;
        unit_wake_cargo(u);
        select_unit(u);
        break;
    case 'w':                               // ask again after the others
        select_unit(u);
        break;
    case 'c':                               // bring it into the middle
        centre_on(unit_ax[u], unit_ay[u]);
        select_follow(u);
        break;
    case 'g':                               // tile select picks where to
        select_begin();
        going = 1;
        message("Go to: Return", "or a click there");
        break;
    default:                                // src/orders.c
        game_order(key);
        break;
    }
}

// The map screen, all of it, as game_run() first puts it up -- after the
// city screen, which wrote over every cell of it. The pixels of the minimap
// are still in place, so only its cells come back; the view is redrawn on
// the next frame, as the patch the city screen drew has replaced part of it.
void draw_screen(void)
{
    screen_clear();
    view_init();
    minimap_show();
    draw_frame();
    draw_status();
    draw_orders();
    dirty = 1;
    hover_dirty = 1;
}

void open_help(byte id)
{
    help_screen(id);
    draw_screen();
}

void open_top_cities(void)
{
    top_cities_screen();
    draw_screen();
}

// A click of the mouse or fire: on the menu bar, the minimap or the map, or
// elsewhere, with the turn over, it ends the turn.
static void click(void)
{
    byte ax, ay;

    if ((byte)(input_y >> 3) == 0) {
        game_orders_menu();
        game_menu_command(menu_click());
        return;
    }
    if (minimap_hex(input_x, input_y, &ax, &ay)) {
        centre_on(ax, ay);
        return;
    }
    if (!pointer_hex(&ax, &ay)) {
        if (unit_active == UNIT_NONE && !game_over) game_end_turn();
        return;
    }
    game_click_hex(ax, ay);
}

// ------------------------------------------------------------- tile select

// Tab: the ring goes on the unit with orders, or the middle of the view.
static void select_begin(void)
{
    word cx = off_x + VIEW_COLUMNS / 2;

    selecting = 1;
    if (unit_active != UNIT_NONE) {
        select_follow(unit_active);
    } else {
        if (cx >= MAP_WIDTH_CHARS) cx -= MAP_WIDTH_CHARS;
        map_cell_at(cx, off_y + VIEW_ROWS / 2, &sel_ax, &sel_ay);
        dirty = 1;
        hover_dirty = 1;
    }
    message("Tile select:", "Tab to leave");
}

void select_end(void)
{
    selecting = 0;
    going = 0;
    message("", "");
    dirty = 1;
    hover_dirty = 1;
}

// One hexagon in direction dir, bringing it into view. Off the top or
// bottom of the map it stays where it is.
static void select_move(byte dir)
{
    byte nx, ny;

    if (!map_neighbour(sel_ax, sel_ay, dir, &nx, &ny)) return;
    sel_ax = nx;
    sel_ay = ny;
    if (!in_view(nx, ny)) centre_on(nx, ny);
    dirty = 1;
    hover_dirty = 1;
}

// A key while tile select is up. Returns 0 for one it leaves to the usual
// handling -- the unit's orders, the menus, HELP.
static byte select_key(byte key)
{
    byte dir = key_dir(key);
    // Odd columns sit half a hexagon lower, so going straight up or down
    // zigzags between a column and the one east of it, and back the same
    // way: every hexagon can be reached with the four cursor keys.
    byte odd = sel_ax & 1;

    switch (key) {
    case KEY_LEFT: dir = DIR_W; break;
    case KEY_RIGHT: dir = DIR_E; break;
    case KEY_UP: dir = odd ? DIR_NW : DIR_NE; break;
    case KEY_DOWN: dir = odd ? DIR_SW : DIR_SE; break;
    case KEY_RETURN: game_click_hex(sel_ax, sel_ay); return 1;
    case KEY_TAB:
    case KEY_ESC: select_end(); return 1;
    }
    if (dir == DIR_NONE) return 0;
    select_move(dir);
    return 1;
}

// Where view_redraw() is to draw the ring, if it is to draw one: one
// hexagon row and a column of the window that may be off either side of
// it, which view_draw_tile() clips.
static void place_mark(void)
{
    int x;

    view_mark_hy = 0xff;
    if (!selecting || sel_hidden) return;
    x = (int)(MAP_CELL_X(sel_ax) + MAP_WIDTH_CHARS - off_x);
    if (x >= MAP_WIDTH_CHARS) x -= MAP_WIDTH_CHARS;
    if (x > MAP_WIDTH_CHARS - TILE_EXPANDED_COLUMNS) x -= MAP_WIDTH_CHARS;
    view_mark_x = x;
    view_mark_hy = 2 * sel_ay + (sel_ax & 1);
}

// --------------------------------------------------------------- the game


// The popup a new game opens with.
byte game_load(void)
{
    return loaded = save_read();
}

// res/text/intro.txt, with the people and their leader filled in.
static void intro(void)
{
    byte civ = player_civ[PLAYER_HUMAN];
    const char *args[2];

    args[0] = civ_people(civ);
    args[1] = civ_leader(civ);
    popup_begin();
    popup_add_text(TEXT_INTRO, args);
    popup_show();
}

byte game_run(void)
{
    byte key;

    fight_shown = 0;
    fight_placed = 0;
    draw_frame();
    minimap_init();
    draw_status();
    select_unit(UNIT_NONE);

    // Over the map rather than over a blank window, so the map is drawn
    // first. A build without the title is for screenshots of the view, and
    // has no popup to get in their way either.
#ifndef NO_TITLE
    view_wait_for_redraw();
    place_mark();
    view_redraw(off_x, off_y, VIEW_ALL);
    minimap_frame(off_x, off_y);
    dirty = 0;
    if (!loaded) intro();
#endif

    while (!quit) {
        byte band = VIEW_ALL;
        // Start the redraw the moment the beam leaves the map window, so
        // that it is rewriting rows the beam has already passed. Without
        // this the redraw lands in the middle of a displayed frame and the
        // map tears while a cursor key is held. Waiting here every frame is
        // also what makes one trip round this loop one frame, which
        // input_poll() counts on.
        view_wait_for_redraw();
        // Anything that changed the view shows the active unit at once and
        // starts its blink again, so that it is there to see as soon as it
        // has moved or been selected, and while the view is scrolled. In
        // tile select the ring blinks instead, and the unit stays.
        if (dirty) {
            unit_hidden = 0;
            sel_hidden = 0;
            blink = 0;
        } else if ((selecting || unit_active != UNIT_NONE)
                   && ++blink == BLINK_FRAMES) {
            if (selecting) {
                sel_hidden ^= 1;
                band = 2 * sel_ay + (sel_ax & 1);
            } else {
                unit_hidden ^= 1;
                band = 2 * unit_ay[unit_active] + (unit_ax[unit_active] & 1);
            }
            blink = 0;
            dirty = 1;
        }
        if (dirty) {
            place_mark();
            view_redraw(off_x, off_y, band);
            minimap_frame(off_x, off_y);
            dirty = 0;
            hover_dirty = 1;
        }
        // Whoever has come in sight is brought into view first, and
        // greeted after the redraw, so that they are on the map behind the
        // greeting -- the popup that said so used to leave the player to
        // find them.
        // The letters wait for the greetings, which come first. Fights
        // come before them all: each is brought into view, and shown
        // after the redraw, the units that died gone from the map; then
        // the view goes back to the unit with the orders.
        if (fight_shown < fight_count) {
            if (!fight_placed) {
                show_at(fight_ax[fight_shown], fight_ay[fight_shown]);
                fight_placed = 1;
            } else {
                fight_show(fight_shown);
                fight_placed = 0;
                if (++fight_shown == fight_count) {
                    fight_shown = fight_count = 0;
                    if (unit_active != UNIT_NONE)
                        centre_on(unit_ax[unit_active], unit_ay[unit_active]);
                }
            }
        } else if (contact) {
            contact = 0;
            greet |= diplomacy_contact();
            if (greet) show_met();
        } else if (greet || diplomacy_waiting) {
            diplomacy_greet(greet);
            greet = 0;
        } else if (end_due) {
            // After the greetings and the letters the last move brought.
            end_due = 0;
            if (!game_end_wait && unit_active == UNIT_NONE && !game_over)
                game_end_turn();
        }

        key = input_poll();
        if (input_moved()) {
            hover_dirty = 1;
            hover_unit = 0;
        }
        if (hover_dirty) {
            game_draw_hover();
            hover_dirty = 0;
        }
        if (!key) continue;
        if (key >= 'A' && key <= 'Z') key += 'a' - 'A';

        // The game is over: the final score, then the title screen.
        if (game_over) {
            score_screen(SCORE_END);
            retired = 1;
            quit = 1;
            continue;
        }
        if (selecting && select_key(key)) continue;

        // Two characters a step across and one row down, because that is
        // the same distance on the glass: at H640/V200 a character is twice
        // as tall as it is wide. A hexagon is 8 characters wide and 3 rows
        // apart, so that is four sub-steps per column and three per row.
        //
        // Across there is no limit: the map wraps east-west, so off_x runs
        // round 0..MAP_WIDTH_CHARS-1 and view_redraw() draws across the seam.
        switch (key) {
        case KEY_LEFT:
            if (off_x < 2) off_x += MAP_WIDTH_CHARS;
            off_x -= 2;
            dirty = 1;
            break;
        case KEY_RIGHT:
            off_x += 2;
            if (off_x >= MAP_WIDTH_CHARS) off_x -= MAP_WIDTH_CHARS;
            dirty = 1;
            break;
        case KEY_UP:
            if (off_y > VIEW_MIN_OFF_Y) { --off_y; dirty = 1; }
            break;
        case KEY_DOWN:
            if (off_y < VIEW_MAX_OFF_Y) { ++off_y; dirty = 1; }
            break;
        case KEY_ESC:
        case KEY_RUNSTOP:
            // Asked, so that a stray Esc does not end the game. Esc again
            // is No.
            if (popup_ask("Do you want to quit to BASIC?")) quit = 1;
            break;
        case KEY_CLICK:
            click();
            break;
        case KEY_TAB:
            select_begin();
            break;
        case KEY_HELP:
            open_help(TEXT_HELP);
            break;
        // The advisors, as the Advisors menu says.
        case KEY_F1:
            game_menu_command(MENU_CITIES);
            break;
        case KEY_F2:
            game_menu_command(MENU_MILITARY);
            break;
        case KEY_F3:
            game_menu_command(MENU_DIPLOMACY);
            break;
        case KEY_F4:
            game_menu_command(MENU_FINANCE);
            break;
        default:
            if (KEY_IS_MEGA(key)) {
                game_orders_menu();
                game_menu_command(menu_key(key));
            }
            else if (unit_active != UNIT_NONE) unit_key(key);
            else if (key == KEY_RETURN) game_end_turn();
            break;
        }
    }

    // The arrow stays for the title screen; the minimap's outline does not.
    sprite_hide(2);
    sprite_hide(3);
    return retired;             // GAME_QUIT, GAME_RETIRE or GAME_LOAD
}
