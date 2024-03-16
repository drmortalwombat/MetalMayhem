#pragma once

#include "levels.h"


bool level_field(char cx, char cy);

bool level_pixel_field(int x, int y);

bool level_sprite_field(int x, int y);

bool level_player_field(int x, int y);

void font_expand(char t, char x, char y);

void map_expand(char pi, char x, char y);

void back_init(void);

#pragma compile("playfield.cpp")
