#ifndef GAMEMUSIC_H
#define GAMEMUSIC_H

enum Tune
{
	TUNE_GAME_1,
	TUNE_GAME_OVER,
	TUNE_GAME_2,
	TUNE_GAME_3,
	TUNE_INTRO,
	TUNE_GAME_4,
};

extern bool music_active;
extern bool system_ntsc;

void music_init(Tune tune);

void music_queue(Tune tune);

void music_play(void);

void music_toggle(void);

void music_fade(void);

void music_patch_voice3(bool enable);

#pragma compile("gamemusic.c")

#endif
