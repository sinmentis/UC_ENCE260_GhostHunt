/** @file   game.c
    @author Shun Lyu && Mike Muldrew
    @date   15 October 2017
    @brief  A simple ghost hunt game. Human can see where ghost is, but ghost move faster.
*/

#include "system.h"
#include "navswitch.h"
#include "pacer.h"
#include "tinygl.h"
#include "../fonts/font3x5_1.h"
#include "ir_uart.h"
#include "led.h"
#include "ledmat.h"
#include "string.h"
#include "button.h"
#include "pio.h"
#include "task.h"
#include <stdbool.h>
#include "display_image.h"

enum {MESSAGE_RATE = 20};
enum {PACER_RATE = 450};
enum {MAP_TASK_RATE = 2800};
enum {DISPLAY_TASK_RATE = 400};
enum {IR_TASK_RATE = 10};
enum {RESULT_TASK_RATE = 200};
enum {unknown_role, human_role, ghost_role};

#define PIEZO1_PIO PIO_DEFINE (PORT_D, 4)
#define PIEZO2_PIO PIO_DEFINE (PORT_D, 6)

/** Define all the image*/
static const uint8_t bitmap[] = {0x21, 0x2C, 0x08, 0x2A, 0x00};
static const uint8_t ghost_map[] = {0x38, 0x57, 0x41, 0x57, 0x38};
static const uint8_t human_map[] = {0x3E, 0x55, 0x45, 0x55, 0x3E};

/** The Walls of map, using to define the edge of the game*/
static const uint8_t map_x[] = {0, 0, 1, 1, 1, 2, 3, 3, 3};
static const uint8_t map_y[] = {0, 5, 2, 3, 5, 3, 1, 3, 5};


/** Struct player contain the state and position*/
struct player_t {
    tinygl_point_t h_pos;
    bool win_lose;
};
typedef struct player_t player;

/** Two role represent two player(Two different functionality)*/
player this;
player other;
int role; // The role of this board selected.

/** Using navswitch to select the role, and push to confirm*/
void select_role(void)
{
    role = unknown_role;
    int tem = unknown_role;
    tinygl_text("SELECT");
    while(role == unknown_role) {
        pacer_wait();
        tinygl_update();
        navswitch_update();
        if(navswitch_push_event_p (NAVSWITCH_NORTH)) {

            tinygl_text_mode_set (TINYGL_TEXT_MODE_STEP);
            tinygl_text("G?");
            tem = ghost_role;

        } else if (navswitch_push_event_p (NAVSWITCH_SOUTH)) {

            tinygl_text_mode_set (TINYGL_TEXT_MODE_STEP);
            tinygl_text("H?");
            tem = human_role;

        } else if (navswitch_push_event_p (NAVSWITCH_PUSH)) {

            role = tem;
        }
    }
}

/** Display the role you selected, push button to start*/
void welcome(void)
{
    tinygl_text_mode_set (TINYGL_TEXT_MODE_SCROLL);
    if(role == human_role) {
        tinygl_text("HUMAN");
    } else if (role == ghost_role) {
        tinygl_text("GHOST");
    }
    int game_on_flag = 0;
    while(game_on_flag == 0) {
        tinygl_update();
        
        // push button to start
        if(pio_input_get(PD7_PIO)) {
            game_on_flag = 1;
        }
        pacer_wait();
    }
}


/** Display map during the game and the result after game, human player shows skull and ghost player shows human face*/
void map_task(__unused__ void *data)
{
    if(this.win_lose == true) {
        display_image(bitmap);
    } else {
        if(role == ghost_role) {
            display_image(human_map);
        } else {
            display_image(ghost_map);
        }
    }
    tinygl_update ();
}

/** return false if there is a wall*/
bool check_edge(uint8_t x, uint8_t y)
{
    int i = 0;
    while(i < 9) {
        if(x == map_x[i] && y == map_y[i]) {
            return false;
        }
        i++;
    }
    return true;
}

/** Move the player, draw ghost for human player*/
static void navswitch_task(__unused__ void *data)
{
    if(this.win_lose == true) {
        led_set(LED1, 1);
        tinygl_point_t newpos;
        navswitch_update();
        newpos = this.h_pos;
        
        // check new position is value or not.
        if(navswitch_push_event_p (NAVSWITCH_NORTH)) {
            if(newpos.y-1 >= 0 && check_edge(newpos.x, newpos.y-1) == true) {
                newpos.y--;
            }
        }
        if(navswitch_push_event_p (NAVSWITCH_SOUTH)) {
            if(newpos.y+1 <= 6 && check_edge(newpos.x, newpos.y+1) == true) {
                newpos.y++;
            }
        }
        if(navswitch_push_event_p (NAVSWITCH_EAST)) {
            if(newpos.x+1 <= 4 && check_edge(newpos.x+1, newpos.y) == true) {
                newpos.x++;
            }
        }
        if(navswitch_push_event_p (NAVSWITCH_WEST)) {
            if(newpos.x-1 >= 0 && check_edge(newpos.x-1, newpos.y) == true) {
                newpos.x--;
            }
        }
        
        // only update when position changed
        if (this.h_pos.x != newpos.x || this.h_pos.y != newpos.y) {
            tinygl_draw_point(this.h_pos, 0);
            this.h_pos = newpos;
            led_set(LED1, 0);
            tinygl_draw_point(this.h_pos, 1);
        }
    }
}

/** Keep sending and receiving player position
 * For human player needs to erase ghost previous positon*/
static void ir_task(__unused__ void *data)
{
    tinygl_point_t newpos;
    newpos = other.h_pos;
    int receive;
    int send;
    if(this.win_lose == true) {
        
        // using special algorithm to decode x and y from one number
        if(ir_uart_read_ready_p()) {
            receive = ir_uart_getc();
            newpos.y = receive % 10;
            newpos.x = (receive % 100 - receive % 10)/10;
            if(role == human_role) {
                tinygl_draw_point(other.h_pos, 0);
            }
            other.h_pos.x = newpos.x;
            other.h_pos.y = newpos.y;

        }
        
        // coded our position so that we only have send one signal rather then 2
        send = this.h_pos.x * 10 + this.h_pos.y;
        ir_uart_putc (send);
    }
}

/** Keep checking whether human player get caught or not*/
static void result_task(__unused__ void* data)
{
    if(this.win_lose == true) {
        if(this.h_pos.x == other.h_pos.x && this.h_pos.y == other.h_pos.y) {
            tinygl_clear(); // ready to display the result
            this.win_lose = false;
        }
    }
}

/**Human got to see the postion of ghost, other can't*/
static void display_task(__unused__ void* data)
{
    if(role == human_role) {
        if(this.win_lose == true) {
            tinygl_draw_point(other.h_pos, 1); // erase the last position of player
        }
    }
    tinygl_update ();
}


/** initilise system*/
void init_all(void)
{
    system_init();

    navswitch_init();

    ledmat_init();

    led_init();

    ir_uart_init();

    button_init();

    pacer_init (PACER_RATE);
    
    tinygl_init(PACER_RATE);
    tinygl_text_speed_set(MESSAGE_RATE);
    tinygl_text_dir_set(TINYGL_TEXT_DIR_ROTATE);
    tinygl_text_mode_set(TINYGL_TEXT_MODE_SCROLL);
    tinygl_font_set(&font3x5_1);
}

int main (void)
{
    init_all();

    select_role();

    welcome();

    uint8_t NAV_TASK_RATE = 10; // deafult speed for human

    /** deafult position and init two player*/
    if (role == human_role) {
        this.h_pos = tinygl_point(2, 0);
        other.h_pos = tinygl_point(2, 6);
    } else {
        other.h_pos = tinygl_point(2, 0);
        this.h_pos = tinygl_point(2, 6);
        NAV_TASK_RATE = 15; // ghost move faster
    }
    this.win_lose = true;
    other.win_lose = true;

    /** Game on*/
    task_t tasks[] = {
        {.func = map_task, .period = TASK_RATE / MAP_TASK_RATE},
        {.func = navswitch_task, .period = TASK_RATE / NAV_TASK_RATE},
        {.func = ir_task, .period = TASK_RATE / IR_TASK_RATE},
        {.func = display_task, .period = TASK_RATE / DISPLAY_TASK_RATE},
        {.func = result_task, .period = RESULT_TASK_RATE},
    };

    tinygl_clear(); // clear all previous display
    tinygl_draw_point(this.h_pos, 1); // display your first location
    task_schedule (tasks, ARRAY_SIZE (tasks));

    return 0;
}
