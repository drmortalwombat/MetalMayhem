#pragma once

#include <c64/rasterirq.h>

// Custom screen address
static char * const Screen = (char *)0xc000;
static char * const Screen2 = (char *)0xc400;

// Custom sprite address
static char * const Sprites = (char *)0xd000;

// Custom intro sprite address
static char * const IntroSprites = (char *)0xe000;

static char * const DynSprites = (char *)0xc400;

// Custom charset address
static char * const Charset = (char *)0xc800;

// Custom dynamic charset address
static char * const DynCharset = Charset + 224 * 8;

// Color mem address
static char * const Color = (char *)0xd800;

extern const char NumberTiles[];

extern const char statx_score[2];
extern const char statx_flag[2];

extern RIRQCode		rirqtop, rirqbottom;

// Init display
void display_init(void);

// Put a character into the stat region
void stats_putchar(char x, char y, char ch);

// Put a character into the game clock region
void clock_put_char(char x, char ch);

// Prepare display of the game frame
void display_game_frame(void);

// Show the game frame
void display_unveil_frame(void);

#pragma compile("display.cpp")

