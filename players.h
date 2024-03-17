#pragma once


enum PlayerID
{
	PLAYER_0,
	PLAYER_1
};

extern __striped struct Player
{
	unsigned	px, py;
	char		dir;
	unsigned	vx, vy;
	signed char	dvx, dvy;
	unsigned	shotx, shoty;
	signed char shotvx, shotvy;
	unsigned	flagx, flagy, hqx, hqy;
	char		shot, explosion, invulnerable;
	signed char	jx, jy;
	bool		jb;
	bool		flag, home;
	unsigned	dscore;

}	players[2];

extern char	score[2][6];

extern unsigned	bolt_x, bolt_y;

void bolt_init(void);

void player_init(PlayerID pi);

void player_init_flag(PlayerID pi);

void player_init_hq(PlayerID pi, unsigned x, unsigned y);

void player_check(PlayerID pi);

void collision_check(void);

void player_ai(PlayerID pi);

void player_control(PlayerID pi);

void player_move(PlayerID pi);

void view_move(PlayerID pi, char phase);

void view_init(PlayerID pi);

void view_scroll(PlayerID pi, char phase);

void view_redraw(PlayerID pi, char phase);

void player_check_score(PlayerID pi, char phase);

void score_init(void);

#pragma compile("players.cpp")

