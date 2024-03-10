#include <string.h>
#include <stdlib.h>
#include <c64/vic.h>
#include <c64/memmap.h>
#include <c64/joystick.h>
#include <c64/sprites.h>
#include <c64/rasterirq.h>
#include <c64/cia.h>
#include <oscar.h>
#include <conio.h>
#include <math.h>

// Custom screen address
char * const Screen = (char *)0xc000;
char * const Screen2 = (char *)0xc400;

// Custom screen address
char * const Sprites = (char *)0xd000;

// Custom charset address
char * const Charset = (char *)0xc800;

// Color mem address
char * const Color = (char *)0xd800;

const char BackgroundTiles[] = {
	#embed ctm_map8 "playfield.ctm"
};

const char SpriteImages[] = {
	#embed spd_sprites lzo "tank.spd"
};

const char NumbersFont[] = {
	#embed ctm_chars lzo "numbers.ctm"
};

char BackgroundMap[40 * 40];

template<int y>
inline void font_expand_2(char t, const char * mp0, const char * mp1)
{
	__assume(t <= 128);
	for(char i=0; i<16; i++)
	{
		char m0 = mp0[i], m1 = mp1[i];
		#pragma unroll(full)
		for(char j=0; j<8; j++)
			Charset[t++] = j < (7 - y) ? m0 : m1;
	}
}

void font_expand(char t, char x, char y)
{
	char m0 = 0xff << x;
	char m1 = ~m0;

	char mt0[16], mt1[16];
	#pragma unroll(full)
	for(char i=0; i<16; i++)
	{
		char w0 = 0, w1 = 0;
		if (i & 1)
			w0 |= m0;
		if (i & 2)
			w0 |= m1;
		if (i & 4)
			w1 |= m0;
		if (i & 8)
			w1 |= m1;
		mt0[i] = w0;
		mt1[i] = w1;
	}

	switch(y)
	{
	case 0: font_expand_2<0>(t, mt0, mt1); break;
	case 1: font_expand_2<1>(t, mt0, mt1); break;
	case 2: font_expand_2<2>(t, mt0, mt1); break;
	case 3: font_expand_2<3>(t, mt0, mt1); break;
	case 4: font_expand_2<4>(t, mt0, mt1); break;
	case 5: font_expand_2<5>(t, mt0, mt1); break;
	case 6: font_expand_2<6>(t, mt0, mt1); break;
	case 7: font_expand_2<7>(t, mt0, mt1); break;		
	}
}

void map_expand(char pi, char x, char y)
{
	const char * sp = BackgroundMap + 40 * y + x;

	char * dp = Screen;
	char 	m = 0;
	if (pi)
	{
		dp += 22;
		m |= 16;
	}

	for(char y=0; y<20; y++)
	{
		#pragma unroll(full)
		for(char x=0; x<18; x++)
			dp[x] = sp[x] | m;

		dp += 40;
		sp += 40;
	}	
}

void back_init(void)
{
	for(char y=0; y<39; y++)
	{
		for(char x=0; x<39; x++)
		{
			char	c0 = BackgroundTiles[40 * y + x];
			char	c1 = BackgroundTiles[40 * y + x + 1];
			char	c2 = BackgroundTiles[40 * y + x + 40];
			char	c3 = BackgroundTiles[40 * y + x + 41];

			char m = 0;
			if (c0)
				m |= 1;
			if (c1)
				m |= 2;
			if (c2)
				m |= 4;
			if (c3)
				m |= 8;

			BackgroundMap[40 * y + x] = m;
		}
	}
}

const signed char sintab16[16] = {
	#for(i, 16) (char)floor(sin(i * PI / 8) * 16.0 + 0.5),
};

const signed char sintab14[16] = {
	#for(i, 16) (char)floor(sin(i * PI / 8) * 14.0 + 0.5),
};

enum PlayerID
{
	PLAYER_0,
	PLAYER_1
};

__striped struct Player
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

char	score[2][6];

void stats_putchar(PlayerID pi, char x, char y, char ch)
{
	char * dp = Screen2 + 40 * 21 + 80 * y + 2 * x;
	if (pi)
		dp += 22;
	dp[ 0] = 128 + 4 * ch;
	dp[ 1] = 129 + 4 * ch;
	dp[40] = 130 + 4 * ch;
	dp[41] = 131 + 4 * ch;
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
	else if (p.hqx > 260)
		p.vx = 164;
	else
		p.vx = ((p.hqx - 100) & ~7) | 4;
	if (p.hqy < 100)
		p.vy = 4;
	else if (p.hqy > 244)
		p.vy = 148;
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
		if (BackgroundTiles[40 * cy + cx])
			e.shot = 0;		
	}

	if (!p.explosion && !p.invulnerable && e.shot)
	{

		unsigned	dx = e.shotx - 48 - p.px;
		unsigned	dy = e.shoty - 32 - p.py;

		if (dx < 256 && dy < 256)
		{
			p.explosion = 16;
			e.shot = 0;
			e.dscore += 200;
		}
	}

	if (!p.explosion && !p.flag)
	{
		if ((p.px >> 4) + 16 - e.flagx < 32 && (p.py >> 4) + 16 - e.flagy < 32)
		{
			p.flag = true;
			stats_putchar(pi, 8, 0, 11);
		}
	}
}

static inline char BackTile(int x, int y)
{
	signed char cx = (x + (12 - 24) * 16) >> 7;
	signed char cy = (y + (11 - 50) * 16) >> 7;

	if (cx < 0 || cy < 0 || cx >= 40 || cy >= 40)
		return 2;
	else
		return BackgroundTiles[cy * 40 + cx];
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
	signed int hy = - sintab16[(dir + 4) & 15] << 3;

	signed int dx = hx << 1;
	signed int dy = hy << 1;

	unsigned	x1 = p.px + dx, y1 = p.py + dy;
	unsigned	x5 = p.px - dx, y5 = p.py - dy;

	unsigned	x0 = x1 + hy, y0 = y1 - hx;
	unsigned	x2 = x1 - hy, y2 = y1 + hx;

	unsigned	x7 = p.px + dy, y7 = p.py - dx;
	unsigned	x3 = p.px - dy, y3 = p.py + dx;

	char ch0 = BackTile(x0, y0);
	char ch1 = BackTile(x1, y1);
	char ch2 = BackTile(x2, y2);
	char ch3 = BackTile(x3, y3);
	char ch7 = BackTile(x7, y7);

	Color[40 * 20 + 20 * pi + 0] = ch0;
	Color[40 * 20 + 20 * pi + 1] = ch1;
	Color[40 * 20 + 20 * pi + 2] = ch2;
	Color[40 * 20 + 20 * pi + 3] = ch3;
	Color[40 * 20 + 20 * pi + 4] = ch7;

	signed char	tx, ty;

	if (p.flag)
	{
		tx = p.hqx >> 3;
		ty = p.hqy >> 3;
	}
	else
	{
		tx = e.flagx >> 3;
		ty = e.flagy >> 3;
	}

	tx -= p.px >> 7;
	ty -= p.py >> 7;

	signed char etx = (e.px - p.px + 64) >> 7;
	signed char ety = (e.py - p.py + 64) >> 7;

	char tdist = idist(tx, ty);
	char edist = idist(etx, ety);

	Screen2[40 * 24 + 20 * pi + 3] = tdist + 0xb0;
	Screen2[40 * 24 + 20 * pi + 4] = edist + 0xb0;

	if (tdist > (p.flag ? 20 : 4))
	{
		if (e.flag || edist < 4)
		{
			tx = etx;
			ty = ety;
			tdist = edist;
		}
	}

	char adir = iatan(ty, tx) ^ 8;
	char edir = iatan(ety, etx) ^ 8;

	char tdiff = (adir - dir) & 15;

	Screen2[40 * 24 + 20 * pi + 0] = dir + 0xb0;
	Screen2[40 * 24 + 20 * pi + 1] = adir + 0xb0;
	Screen2[40 * 24 + 20 * pi + 2] = tdiff + 0xb0;

	if (!(p.dir & 7))
		p.jx = 0;

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

	p.jy = -1;
	if (p.jx == 0 && !(rand() & 31) && tdiff > 4 && tdiff < 12)
	{
		if (tdiff < 8)
			p.jx = -1;
		else
			p.jx = 1;
		p.jy = 0;
	}

	p.jb = false;
	if (edist < 8 && edir == dir)
		p.jb = true;

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

	if (p.explosion)
	{
		p.explosion--;

		if (p.explosion == 0)
		{
			if (p.flag)
			{
				p.flag = false;
				players[1 - pi].flagx = p.px >> 4;
				players[1 - pi].flagy = p.py >> 4;
				stats_putchar(pi, 8, 0, 0);
			}
			player_init(pi);
		}
	}
	else
	{
		p.dir = (p.dir - p.jx) & 63;

		char dir = p.dir >> 2;

		if (p.invulnerable && !p.home)
			p.invulnerable--;

		signed char dx = 0, dy = 0;
		if (p.jy > 0)
		{
			dx = sintab14[dir];
			dy = sintab14[(dir + 4) & 15];
		}
		else if (p.jy < 0) 
		{
			if (p.flag)
			{
				dx = -sintab14[dir];
				dy = -sintab14[(dir + 4) & 15];
			}
			else
			{
				dx = -sintab16[dir];
				dy = -sintab16[(dir + 4) & 15];
			}
		}

		unsigned	tx = p.px + dx;
		unsigned	ty = p.py + dy;

		unsigned cx = (tx - 20 * 16) >> 7;
		unsigned cy = (ty - 46 * 16) >> 7;

		if (dx < 0)
		{
			if (BackgroundTiles[40 * cy + cx] || 
				BackgroundTiles[40 * cy + cx + 40])
				tx -= dx;
		}
		else if (dx > 0)
		{
			if (BackgroundTiles[40 * cy + cx + 2] || 
				BackgroundTiles[40 * cy + cx + 40 + 2])
				tx -= dx;
		}
		if (dy < 0)
		{
			if (BackgroundTiles[40 * cy + cx] || 
				BackgroundTiles[40 * cy + cx + 1])
				ty -= dy;
		}
		else if (dy > 0)
		{
			if (BackgroundTiles[40 * cy + cx + 80] || 
				BackgroundTiles[40 * cy + cx + 1 + 80])
				ty -= dy;
		}

		p.px = tx;
		p.py = ty;

		if (!p.shot && p.jb)
		{
			p.shot = 32;
			p.shotvx = - 2 * sintab16[dir];
			p.shotvy = - 2 * sintab16[(dir + 4) & 15];
			p.shotx = p.px + 2 * p.shotvx + 160;
			p.shoty = p.py + 2 * p.shotvy + 128;			
		}

		if ((p.px >> 4) + 16 - p.hqx < 32 && (p.py >> 4) + 16 - p.hqy < 32)
		{
			p.home = true;
			if (p.flag)
			{
				p.flag = false;
				p.dscore += 1000;
				player_init_flag(1 - pi);
				stats_putchar(pi, 8, 0, 0);
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

	if (players[0].explosion)
		vspr_image(pi * 8    , 95 - (players[0].explosion >> 1));
	else if (players[0].invulnerable && (phase & 2))
		vspr_image(pi * 8    , 87);
	else
		vspr_image(pi * 8    , 64 + (players[0].dir >> 2));

	if (players[1].explosion)
		vspr_image(pi * 8 + 1, 95 - (players[1].explosion >> 1));
	else if (players[1].invulnerable && (phase & 2))
		vspr_image(pi * 8 + 1, 87);
	else
		vspr_image(pi * 8 + 1, 64 + (players[1].dir >> 2));

	if ((players[0].px >> 4) + 16 - players[0].hqx < 32 && (players[0].py >> 4) + 16 - players[0].hqy < 32)
		vspr_hide(pi * 8 + 2);
	else
		view_sprite(pi, 2, players[0].hqx - p.vx, players[0].hqy - p.vy);

	if ((players[1].px >> 4) + 16 - players[1].hqx < 32 && (players[1].py >> 4) + 16 - players[1].hqy < 32)
		vspr_hide(pi * 8 + 3);
	else
		view_sprite(pi, 3, players[1].hqx - p.vx, players[1].hqy - p.vy);

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
		else if (p.vx < 160 && tx > p.vx + 128)
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
		else if (p.vy < 144 && ty > p.vy + 144)
		{
			p.dvy = 1;
			p.vy = (p.vy & ~7) | 4;
		}
		else
			p.dvy = 0;		
	}
}

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
			stats_putchar(pi, ci, 0, 1 + score[pi][ci]);
			ci--;
			score[pi][ci]++;
		}
		stats_putchar(pi, ci, 0, 1 + score[pi][ci]);
	}
}

RIRQCode		rirqtop, rirqbottom;

int main(void)
{
	cia_init();

	// Install memory trampoline to ensure system
	// keeps running when playing with the PLA
	mmap_trampoline();

	// Swap in all RAM
	mmap_set(MMAP_RAM);

	oscar_expand_lzo(Charset + 1024, NumbersFont);
	oscar_expand_lzo(Sprites, SpriteImages);

	// Swap ROM back in
	mmap_set(MMAP_NO_ROM);

	memset(Screen, 15, 40 * 21);
	memset(Color, VCOL_MED_GREY, 1000);

	for(char y=0; y<20; y++)
	{
		for(char x=18; x<22; x++)
			Color[40 * y + x] = VCOL_DARK_GREY;
	}

	memset(Color + 40 * 20, VCOL_DARK_GREY, 40 * 5);
	memset(Color + 40 * 21, VCOL_WHITE, 40 * 4);

	memset(Screen2 + 40 * 20, 15, 40);
	memset(Screen2 + 40 * 21, 128, 40 * 4);

	for(char i=0; i<8; i++)
		Screen2[0x3f8 + i] = 87;

	// Background and border black
	vic.color_border = VCOL_DARK_GREY;
	vic.color_back = VCOL_BLACK;
	vic.spr_multi = 0x00;
	vic.spr_priority = 0xff;

	// Set VIC to show custom screen with custom charset
	vic_setmode(VICM_TEXT, Screen, Charset);

	rirq_init_io();

	back_init();

	vspr_init(Screen);

	vspr_set(0, 0, 0, 64, VCOL_YELLOW);
	vspr_set(1, 0, 0, 64, VCOL_RED);
	vspr_set(2, 0, 0, 84, VCOL_YELLOW);
	vspr_set(3, 0, 0, 84, VCOL_RED);
	vspr_set(4, 0, 0, 80, VCOL_YELLOW);
	vspr_set(5, 0, 0, 80, VCOL_RED);
	vspr_set(6, 0, 0, 85, VCOL_YELLOW);
	vspr_set(7, 0, 0, 85, VCOL_RED);

	vspr_set(8, 0, 0, 64, VCOL_YELLOW);
	vspr_set(9, 0, 0, 64, VCOL_RED);
	vspr_set(10, 0, 0, 84, VCOL_YELLOW);
	vspr_set(11, 0, 0, 84, VCOL_RED);
	vspr_set(12, 0, 0, 80, VCOL_YELLOW);
	vspr_set(13, 0, 0, 80, VCOL_RED);
	vspr_set(14, 0, 0, 85, VCOL_YELLOW);
	vspr_set(15, 0, 0, 85, VCOL_RED);

	rirq_build(&rirqbottom, 2);
	rirq_write(&rirqbottom, 0, &vic.memptr, 0x12);
	rirq_write(&rirqbottom, 1, &vic.color_back, VCOL_DARK_GREY);
	rirq_set(9, 54 + 20 * 8, &rirqbottom);

	rirq_build(&rirqtop, 2);
	rirq_write(&rirqtop, 0, &vic.memptr, 0x02);
	rirq_write(&rirqtop, 1, &vic.color_back, VCOL_BLACK);
	rirq_set(10, 40, &rirqtop);

	vspr_sort();
	vspr_update();
	rirq_sort();

	rirq_start();

	player_init_hq(PLAYER_0, 32, 58);
	player_init_hq(PLAYER_1, 304, 330);
	player_init_flag(PLAYER_0);
	player_init_flag(PLAYER_1);
	player_init(PLAYER_0);
	player_init(PLAYER_1);

	for(char i=0; i<5; i++)
	{
		stats_putchar(PLAYER_0, i, 0, 1);
		stats_putchar(PLAYER_1, i, 0, 1);
		score[0][i] = score[1][i] = 0;
	}

	char	phase = 0;
	while (true)
	{
		view_move(PLAYER_0, phase);
		view_move(PLAYER_1, phase);

		vspr_sort();

		rirq_wait();

		vspr_update();
		rirq_sort();

//		vic.color_border++;
		view_scroll(PLAYER_0, phase & 7);
		view_scroll(PLAYER_1, (phase + 4) & 7);
//		vic.color_border++;
		view_redraw(PLAYER_0, phase & 7);
		view_redraw(PLAYER_1, (phase + 4) & 7);
//		vic.color_border = VCOL_DARK_GREY;

		char flgani = 80 + ((phase >> 2) & 3); 

		vspr_image(4, flgani);
		vspr_image(5, flgani);
		vspr_image(12, flgani);
		vspr_image(13, flgani);

		phase++;

		player_check(PLAYER_0);
		player_check(PLAYER_1);

		player_control(PLAYER_0);
//		player_control(PLAYER_1);

//		player_ai(PLAYER_0);
		player_ai(PLAYER_1);

		player_move(PLAYER_0);
		player_move(PLAYER_1);

		player_check_score(PLAYER_0, phase);
		player_check_score(PLAYER_1, phase);
	}

	return 0;
}
