#pragma once

#include "levels.h"

extern const LevelData * intro_select_level;
extern bool intro_select_players;
extern char intro_select_level_index;

void intro_init(void);

void intro_char(char x, char y, char ch, char color1, char color2);

void intro_string(char x, char y, const char * str, char color1, char color2);

#pragma compile("intro.cpp")
