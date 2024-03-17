#pragma once

#include "levels.h"

extern const LevelData * intro_select_level;
extern bool intro_select_players;

void intro_init(void);

#pragma compile("intro.cpp")
