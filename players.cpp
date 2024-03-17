#include "players.h"
#include "soundfx.h"
#include <math.h>
#include <c64/joystick.h>
#include <c64/sprites.h>
#include <c64/keyboard.h>

#define DEBUG_AI	0


__striped struct Player	players[2];

unsigned	bolt_x, bolt_y;


const signed char sintab16[20] = {
	#for(i, 20) (signed char)floor(sin(i * PI / 8) * 16.0 + 0.5),
};

__striped const int sintab128[20] = {
	#for(i, 20) (int)floor(sin(i * PI / 8) * 128.0 + 0.5),
};

const signed char sintab14[20] = {
	#for(i, 20) (signed char)floor(sin(i * PI / 8) * 14.0 + 0.5),
};

const signed char sintab112[20] = {
	#for(i, 20) (signed char)floor(sin(i * PI / 8) * 112.0 + 0.5),
};


void bolt_init(void)
{
	do {
		bolt_x = rand() & 511;
		bolt_y = rand() & 511;
	} while (
		level_sprite_field(bolt_x + 4, bolt_y + 4) ||
		level_sprite_field(bolt_x + 19, bolt_y + 4) ||
		level_sprite_field(bolt_x + 4, bolt_y + 19) ||
		level_sprite_field(bolt_x + 19, bolt_y + 19));	
}

void player_init(PlayerID pi)
{
	auto & p = players[pi];
	p.px = p.hqx << 4;
	p.py = p.hqy << 4;
	p.dir = 40 + pi * 32;
	p.dvx = 0;
	p.dvy = 0;

	if (p.hqx < 100)
		p.vx = 4;
	else if (p.hqx > 332)
		p.vx = 236;
	else
		p.vx = ((p.hqx - 100) & ~7) | 4;

	if (p.hqy < 100)
		p.vy = 4;
	else if (p.hqy > 268)
		p.vy = 172;
	else
		p.vy = ((p.hqy - 100) & ~7) | 4;

	p.shot = 0;
	p.explosion = 0;
	p.invulnerable = 100;
	p.flag = false;
	p.home = true;

	font_expand(pi * 128, p.vx & 7, p.vy & 7);
	map_expand(pi, p.vx >> 3, p.vy >> 3);
}

void player_init_flag(PlayerID pi)
{
	auto & p = players[pi];

	p.flagx = p.hqx + 32 - 64 * pi;
	p.flagy = p.hqy;
}

void player_init_hq(PlayerID pi, unsigned x, unsigned y)
{
	players[pi].hqx = x;
	players[pi].hqy = y;
}

void player_check(PlayerID pi)
{
	auto & p = players[pi];
	auto & e = players[1 - pi];

	if (e.shot)
	{
		e.shotx += e.shotvx;
		e.shoty += e.shotvy;
		e.shot--;

		unsigned cx = (e.shotx - 22 * 16) >> 7;
		unsigned cy = (e.shoty - 48 * 16) >> 7;
		if (level_field(cx, cy))
			e.shot = 0;		
	}

	if (!p.explosion && !p.invulnerable && e.shot)
	{

		unsigned	dx = e.shotx - 48 - p.px;
		unsigned	dy = e.shoty - 32 - p.py;

		if (dx < 256 && dy < 256)
		{
			p.explosion = 114;
			e.shot = 0;
			e.dscore += 200;
			sfx_explosion();
		}
	}

	if (!p.explosion && !p.flag)
	{
		if ((p.px >> 4) + 16 - e.flagx < 32 && (p.py >> 4) + 16 - e.flagy < 32)
		{
			p.flag = true;
			stats_putchar(statx_flag[pi], 0, 13);
			sfx_flag_capture();
		}
	}

	if (!p.explosion)
	{
		if ((p.px >> 4) + 16 - bolt_x < 32 && (p.py >> 4) + 16 - bolt_y < 32)
		{
			sfx_katching();
			p.dscore += 100;
			bolt_init();
		}		
	}
}

void collision_check(void)
{
	auto & p = players[0];
	auto & e = players[1];

	if (!e.explosion && !p.explosion)
	{
		if ((int)(p.px - e.px) >= -192 && (int)(p.px - e.px) < 192 &&
			(int)(p.py - e.py) >= -192 && (int)(p.py - e.py) < 192)
		{
			if (!p.invulnerable)
			{
				p.explosion = 114;
				sfx_explosion();
			}
			if (!e.invulnerable)
			{
				e.explosion = 114;
				sfx_explosion();
			}
		}
	}
}

char uatan(char tx, char ty)
{
	if (ty < tx)
	{
		if (5 * ty < tx)
			return 0;
		else if (8 * ty < 5 * tx)
			return 1;
		else
			return 2;
	}
	else
	{
		if (5 * tx < ty)
			return 4;
		else if (8 * tx < 5 * ty)
			return 3;
		else
			return 2;		
	}
}

char iatan(signed char tx, signed char ty)
{
	if (tx < 0)
	{
		if (ty < 0)
			return 8 + uatan(-tx, -ty);
		else
			return 8 - uatan(-tx, ty);				
	}
	else if (ty < 0)
	{
		return (16 - uatan(tx, -ty)) & 15;
	}
	else
		return uatan(tx, ty);
}

char idist(signed char tx, signed char ty)
{
	if (tx < 0)
		tx = -tx;
	if (ty < 0)
		ty = -ty;
	if (tx > ty)
		return tx;
	else
		return ty;
}

void player_ai(PlayerID pi)
{
	auto & p = players[pi];
	auto & e = players[1 - pi];

	char dir = p.dir >> 2;

	signed int hx = - sintab16[dir] << 3;
	signed int hy = - sintab16[dir + 4] << 3;

	signed int dx = hx << 1;
	signed int dy = hy << 1;

	unsigned	x1 = p.px + dx, y1 = p.py + dy;
	unsigned	x5 = p.px - dx, y5 = p.py - dy;

	unsigned	x0 = x1 + hy, y0 = y1 - hx;
	unsigned	x2 = x1 - hy, y2 = y1 + hx;

	unsigned	x7 = p.px + dy, y7 = p.py - dx;
	unsigned	x3 = p.px - dy, y3 = p.py + dx;

	bool ch0 = level_player_field(x0, y0);
	bool ch1 = level_player_field(x1, y1);
	bool ch2 = level_player_field(x2, y2);
	bool ch3 = level_player_field(x3, y3);
	bool ch7 = level_player_field(x7, y7);

#if DEBUG_AI
	Color[40 * 20 + 20 * pi + 0] = ch0;
	Color[40 * 20 + 20 * pi + 1] = ch1;
	Color[40 * 20 + 20 * pi + 2] = ch2;
	Color[40 * 20 + 20 * pi + 3] = ch3;
	Color[40 * 20 + 20 * pi + 4] = ch7;
#endif

	signed char	tx, ty;
	
	signed char bx = (bolt_x - (p.px >> 4) + 4) >> 3;
	signed char by = (bolt_y - (p.py >> 4) + 4) >> 3;

	if (p.flag)
	{
		tx = (p.hqx - (p.px >> 4) + 4) >> 3;
		ty = (p.hqy - (p.py >> 4) + 4) >> 3;
	}
	else
	{
		tx = (e.flagx - (p.px >> 4) + 4) >> 3;
		ty = (e.flagy - (p.py >> 4) + 4) >> 3;
	}

	signed char etx = (e.px - p.px + 64) >> 7;
	signed char ety = (e.py - p.py + 64) >> 7;

	char tdist = idist(tx, ty);
	char edist = idist(etx, ety);
	char bdist = idist(bx, by);

#if DEBUG_AI
	Screen2[40 * 24 + 20 * pi + 3] = tdist + 0xb0;
	Screen2[40 * 24 + 20 * pi + 4] = edist + 0xb0;
	Screen2[40 * 24 + 20 * pi + 5] = (p.dir & 7) + 0xb0;
#endif

	if (tdist > (p.flag ? 20 : 6))
	{
		if (e.invulnerable)
		{
 			if (!p.flag)
			{
				tx = bx;
				ty = by;
				tdist = bdist;
			}
		}
		else if (e.flag || edist < 6)
		{
			tx = etx;
			ty = ety;
			tdist = edist;
		}
		else if (bdist < 8)
		{
			tx = bx;
			ty = by;
			tdist = bdist;
		}
	}

	char adir = iatan(ty, tx) ^ 8;
	char edir = iatan(ety, etx) ^ 8;

	char tdiff = (adir - dir) & 15;

#if DEBUG_AI
	Screen2[40 * 24 + 20 * pi + 0] = dir + 0xb0;
	Screen2[40 * 24 + 20 * pi + 1] = adir + 0xb0;
	Screen2[40 * 24 + 20 * pi + 2] = tdiff + 0xb0;
#endif

	if (p.jx != 0 && p.jy == 0 && tdiff >= 3 && tdiff <= 13)
	{

	}
	else
	{
		if ((p.dir & 7) == 2)
			p.jx = 0;

		p.jy = -1;
		if (ch0 && ch2)
		{
			if (p.jx == 0)
			{
				if (!ch7 && (tdiff > 0 && tdiff < 8))
					p.jx = -1;
				else if (!ch3 && tdiff > 8)
					p.jx = 1;
				else if (ch3)
					p.jx = -1;
				else
					p.jx = 1;
			}
		}
		else if (ch0)
			p.jx = 1;
		else if (ch2)
			p.jx = -1;
		else if (!ch0 && !ch7 && (tdiff > 1 && tdiff < 8))
			p.jx = -1;
		else if (!ch2 && !ch3 && (tdiff > 8 && tdiff < 15))
			p.jx = 1;
		else if (ch1 && !p.jx)
			p.jx = (rand() & 2) - 1;

		if (p.jx == 0 && !(rand() & 31) && tdiff > 6 && tdiff < 10)
		{
			if (tdiff < 8)
				p.jx = -1;
			else
				p.jx = 1;
			p.jy = 0;
		}
	}

	if (e.explosion)
		p.jb = false;
	else
	{
		char	endir = e.dir >> 2;
		char	rdir = (edir - dir) & 15;
		char	enrdir = (endir - dir) & 15;

		if (edist < 3 && rdir == 0)
			p.jy = 1;
		else if (edist < 5 && (rdir <= 1 || rdir >= 15) && enrdir >= 6 && enrdir <= 10)
		{
			p.jy = 1;
			if (rdir == 1)
				p.jx = -1;
			else if (rdir == 15)
				p.jx = 1;
		}

		p.jb = false;
		if (edist < 8 && edir == dir)
			p.jb = true;
	}

}

void player_control(PlayerID pi)
{
	auto & p = players[pi];

	joy_poll(pi);

	char dir = p.dir;
	char tdir = dir;

	if (joyx[pi] > 0)
	{
		if (joyy[pi] < 0)
			tdir = 14 * 4;
		else if (joyy[pi] == 0)
			tdir = 12 * 4;
		else
			tdir = 10 * 4;
	}
	else if (joyx[pi] == 0)
	{
		if (joyy[pi] < 0)
			tdir = 0 * 4;
		else if (joyy[pi] > 0)
			tdir = 8 * 4;
	}
	else
	{
		if (joyy[pi] < 0)
			tdir = 2 * 4;
		else if (joyy[pi] == 0)
			tdir = 4 * 4;
		else
			tdir = 6 * 4;
	}

	tdir = (tdir - dir) & 63;

	if (tdir > 0 && tdir < 32)
		p.jx = -1;
	else if (tdir > 32)
		p.jx = 1;
	else
		p.jx = 0;

	if (joyx[pi] || joyy[pi])
	{
		if (tdir <= 4 || tdir >= 60)
			p.jy = -1;
		else if (tdir >= 28 && tdir <= 36)
			p.jy = 1;
		else
			p.jy = 0;
	}
	else
		p.jy = 0;

	p.jb = joyb[pi];
}

void player_move(PlayerID pi)
{
	auto & p = players[pi];
	auto & e = players[1 - pi];

	if (p.explosion)
	{
		p.explosion--;

		if (p.explosion == 0)
		{
			player_init(pi);
		}
		else if (p.explosion == 98)
		{
			if (p.flag)
			{
				p.flag = false;
				players[1 - pi].flagx = p.px >> 4;
				players[1 - pi].flagy = p.py >> 4;
				stats_putchar(statx_flag[pi], 0, 0);
			}			
		}
	}
	else
	{
		p.dir = (p.dir - p.jx) & 63;

		char dir = p.dir >> 2;

		if (p.invulnerable && !p.home)
			p.invulnerable--;

		signed char dx14 = sintab14[dir];
		signed char dy14 = sintab14[dir + 4];
		signed char dx16 = sintab16[dir];
		signed char dy16 = sintab16[dir + 4];
		signed char dx112 = sintab112[dir];
		signed char dy112 = sintab112[dir + 4];
		int dx128 = sintab128[dir];
		int dy128 = sintab128[dir + 4];

		signed char dx = 0, dy = 0;

		if (p.jy > 0)
		{
			dx = dx14;
			dy = dy14;
		}
		else if (p.jy < 0) 
		{
			if (p.flag)
			{
				dx = -dx14;
				dy = -dy14;
			}
			else
			{
				dx = -dx16;
				dy = -dy16;
			}
		}

		unsigned	tx = p.px + dx;
		unsigned	ty = p.py + dy;

		unsigned	txc	= tx, tyc = ty;
		unsigned	txf = tx, tyf = ty;

		if (p.jy > 0)
		{
			txc += dx112;
			tyc += dy112;
			txf += dx112;
			tyf += dy112;
		}
		else
		{
			txc -= dx128;
			tyc -= dy128;
			txf -= dx112;
			tyf -= dy112;
		}		

		unsigned	tx0 = txf - dy112;
		unsigned	ty0 = tyf + dx112;

		unsigned	tx1 = txf + dy112;
		unsigned	ty1 = tyf - dx112;

		bool	f0 = level_player_field(tx0, ty0);
		bool	fc = level_player_field(txc, tyc);
		bool	f1 = level_player_field(tx1, ty1);

		if (fc || (f0 && f1))
		{
			tx -= dx;
			ty -= dy;
		}
		else if (f0)
		{
			bool	bx = level_player_field(tx0 + dy16, ty0);
			bool	by = level_player_field(tx0, ty0 - dx16);

			if (!bx)
				tx += dy16;
			else if (!by)
				ty -= dx16;
			else
			{
				tx += dy16;
				ty -= dx16;				
			}
		}
		else if (f1)
		{
			bool	bx = level_player_field(tx1 - dy16, ty1);
			bool	by = level_player_field(tx1, ty1 + dx16);

			if (!bx)
				tx -= dy16;
			else if (!by)
				ty += dx16;
			else
			{
				tx -= dy16;
				ty += dx16;				
			}
		}

		p.px = tx;
		p.py = ty;

		if (!p.shot && p.jb)
		{
			p.shot = 32;
			p.shotvx = - 2 * sintab16[dir];
			p.shotvy = - 2 * sintab16[dir + 4];
			p.shotx = p.px + 2 * p.shotvx + 160;
			p.shoty = p.py + 2 * p.shotvy + 128;			
			sfx_shot();
		}

		if ((p.px >> 4) + 16 - p.hqx < 32 && (p.py >> 4) + 16 - p.hqy < 32)
		{
			p.home = true;
			if (p.flag)
			{
				p.flag = false;
				p.dscore += 1000;
				player_init_flag(1 - pi);
				stats_putchar(statx_flag[pi], 0, 0);
				sfx_flag_arrival();
			}
		}
		else
			p.home = false;
	}
}

void view_sprite(PlayerID pi, char vi, int sx, int sy)
{
	if (sx > 0 && sy > 29 && sx < 168 && sy < 210)
		vspr_move(pi * 8 + vi, sx  + 22 * 8 * pi, sy);
	else
		vspr_hide(pi * 8 + vi);
}

void view_move(PlayerID pi, char phase)
{
	auto & p = players[pi];

	p.vx += p.dvx;
	p.vy += p.dvy;

	view_sprite(pi, 0, (players[0].px >> 4) - p.vx, (players[0].py >> 4) - p.vy);
	view_sprite(pi, 1, (players[1].px >> 4) - p.vx, (players[1].py >> 4) - p.vy);

	if (players[0].explosion >= 98)
		vspr_image(pi * 8    , 95 - ((players[0].explosion - 98) >> 1));
	else if (players[0].explosion)
		vspr_hide(pi * 8);
	else if (players[0].invulnerable && (phase & 2))
		vspr_image(pi * 8    , 87);
	else
		vspr_image(pi * 8    , 64 + (players[0].dir >> 2));

	if (players[1].explosion >= 98)
		vspr_image(pi * 8 + 1, 95 - ((players[1].explosion - 98) >> 1));
	else if (players[1].explosion)
		vspr_hide(pi * 8 + 1);
	else if (players[1].invulnerable && (phase & 2))
		vspr_image(pi * 8 + 1, 87);
	else
		vspr_image(pi * 8 + 1, 64 + (players[1].dir >> 2));

	if (p.vx < 16 * 8)
	{
		vspr_color(pi * 8 + 2, VCOL_YELLOW);
		if ((players[0].px >> 4) + 16 - players[0].hqx < 32 && (players[0].py >> 4) + 16 - players[0].hqy < 32)
			vspr_hide(pi * 8 + 2);
		else
			view_sprite(pi, 2, players[0].hqx - p.vx, players[0].hqy - p.vy);
	}
	else
	{
		vspr_color(pi * 8 + 2, VCOL_LT_BLUE);
		if ((players[1].px >> 4) + 16 - players[1].hqx < 32 && (players[1].py >> 4) + 16 - players[1].hqy < 32)
			vspr_hide(pi * 8 + 2);
		else
			view_sprite(pi, 2, players[1].hqx - p.vx, players[1].hqy - p.vy);
	}

	view_sprite(pi, 3, bolt_x - p.vx, bolt_y - p.vy);

	if (players[1].flag)
		vspr_hide(pi * 8 + 4);
	else
		view_sprite(pi, 4, players[0].flagx - p.vx, players[0].flagy - p.vy);

	if (players[0].flag)
		vspr_hide(pi * 8 + 5);
	else
		view_sprite(pi, 5, players[1].flagx - p.vx, players[1].flagy - p.vy);

	if (players[0].shot)
		view_sprite(pi, 6, (players[0].shotx >> 4) - p.vx, (players[0].shoty >> 4) - p.vy);
	else
		vspr_hide(pi * 8 + 6);
	if (players[1].shot)
		view_sprite(pi, 7, (players[1].shotx >> 4) - p.vx, (players[1].shoty >> 4) - p.vy);
	else
		vspr_hide(pi * 8 + 7);
}

void view_init(PlayerID pi)
{
	auto & p = players[pi];

	font_expand(pi * 128, p.vx & 7, p.vy & 7);	
	map_expand(pi, p.vx >> 3, p.vy >> 3);
	view_move(pi, 0);
}

void view_scroll(PlayerID pi, char phase)
{
	auto & p = players[pi];

	if (p.dvx || p.dvy)
		font_expand(pi * 128, p.vx & 7, p.vy & 7);
}

void view_redraw(PlayerID pi, char phase)
{
	auto & p = players[pi];

	if (p.dvx || p.dvy)
	{
		if (phase == 0)
			map_expand(pi, p.vx >> 3, p.vy >> 3);
	}

	if (phase == 4)
	{
		unsigned tx = p.px >> 4;
		unsigned ty = p.py >> 4;

		if (p.vx > 8 && tx < p.vx + 64)
		{
			p.dvx = -1;
			p.vx = (p.vx & ~7) | 3;
		}
		else if (p.vx < 232 && tx > p.vx + 112)
		{
			p.dvx = 1;
			p.vx = (p.vx & ~7) | 4;
		}
		else
			p.dvx = 0;		

		if (p.vy > 8 && ty < p.vy + 88)
		{
			p.dvy = -1;
			p.vy = (p.vy & ~7) | 3;
		}
		else if (p.vy < 168 && ty > p.vy + 144)
		{
			p.dvy = 1;
			p.vy = (p.vy & ~7) | 4;
		}
		else
			p.dvy = 0;		
	}
}

char	score[2][6];

void player_check_score(PlayerID pi, char phase)
{	
	auto & p = players[pi];

	if (!p.explosion && !p.invulnerable)
	{
		if (p.flag && !(phase & 3))
			p.dscore++;
		else if (!(phase & 15))
			p.dscore++;
	}

	if (p.dscore)
	{
		char	ci = 4;
		if (p.dscore >= 1000)
		{
			score[pi][1]++;
			p.dscore -= 1000;
			ci = 1;
		}
		else if (p.dscore >= 100)
		{
			score[pi][2]++;
			p.dscore -= 100;
			ci = 2;
		}
		else if (p.dscore >= 10)
		{
			score[pi][3]++;
			p.dscore -= 10;
			ci = 3;
		}
		else
		{
			score[pi][4] += p.dscore;
			p.dscore = 0;
		}

		while (score[pi][ci] > 9)
		{
			score[pi][ci] -= 10;
			stats_putchar(statx_score[pi] + ci, 0, '0' + score[pi][ci]);
			ci--;
			score[pi][ci]++;
		}
		stats_putchar(statx_score[pi] + ci, 0, '0' + score[pi][ci]);
	}
}

void score_init(void)
{
	for(char i=0; i<5; i++)
	{
		stats_putchar(statx_score[PLAYER_0] + i, 0, '0');
		stats_putchar(statx_score[PLAYER_1] + i, 0, '0');
		score[0][i] = score[1][i] = 0;
	}
	stats_putchar(statx_flag[PLAYER_0], 0, 0);
	stats_putchar(statx_flag[PLAYER_1], 0, 0);
}

