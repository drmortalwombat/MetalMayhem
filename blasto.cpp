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

// Custom screen address
char * const Sprites = (char *)0xd000;

// Custom charset address
char * const Charset = (char *)0xc800;

// Color mem address
char * const Color = (char *)0xd800;

// The charpad resource in lz compression, without the need
// for binary export
const char BackgroundFont[] = {
	#embed ctm_chars lzo "playfield.ctm"
};

const char BackgroundTiles[] = {
	#embed ctm_map8 "playfield.ctm"
};

const char SpriteImages[] = {
	#embed spd_sprites lzo "tank.spd"
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

}	players[2];

void player_init(char pi)
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

	font_expand(pi * 128, p.vx & 7, p.vy & 7);
	map_expand(pi, p.vx >> 3, p.vy >> 3);
}

void player_init_flag(char pi, unsigned x, unsigned y)
{
	players[pi].flagx = x;
	players[pi].flagy = y;
}

void player_init_hq(char pi, unsigned x, unsigned y)
{
	players[pi].hqx = x;
	players[pi].hqy = y;
}


void player_check(char pi)
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

	if (!p.explosion && e.shot)
	{

		unsigned	dx = e.shotx - 48 - p.px;
		unsigned	dy = e.shoty - 32 - p.py;

		if (dx < 256 && dy < 256)
		{
			p.explosion = 16;
			e.shot = 0;
		}
	}
}

static inline char BackTile(unsigned x, unsigned y)
{
	return BackgroundTiles[((y + (11 - 50)  * 16) >> 7) * 40 + ((x + (12 - 24) * 16) >> 7)];
}

void player_ai(char pi)
{
	auto & p = players[pi];

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

	if (!(p.dir & 15))
		p.jx = 0;

	if (ch1)
	{
		if (ch3)
			p.jx = -1;
		else if (ch7)
			p.jx = 1;
		else if (!p.jx)
			p.jx = (rand() & 2) - 1;
	}
	else if (!p.jx)
	{
		if (ch2 && !ch7)
			p.jx = -1;
		else if (ch0 && !ch3)
			p.jx = 1;
		else if (!(rand() & 255))
		{
			if (!ch7)
				p.jx = -1;
			else if (!ch3)
				p.jx = 1;
		}
	}

	p.jy = -1;
	p.jb = false;
}

void player_control(char pi)
{
	auto & p = players[pi];

	joy_poll(pi);
	p.jx = joyx[pi];
	p.jy = joyy[pi];
	p.jb = joyb[pi];
}

void player_move(char pi)
{
	auto & p = players[pi];

	if (p.explosion)
	{
		p.explosion--;

		vspr_image(pi, 95 - (p.explosion >> 1));
		vspr_image(pi + 8, 95 - (p.explosion >> 1));
		if (p.explosion == 0)
			player_init(pi);
	}
	else
	{
		p.dir = (p.dir - p.jx) & 63;

		char dir = p.dir >> 2;

		if (p.invulnerable)
			p.invulnerable--;

		if (p.invulnerable & 1)
		{
			vspr_image(pi    , 87);
			vspr_image(pi + 8, 87);
		}
		else
		{
			vspr_image(pi    , 64 + dir);
			vspr_image(pi + 8, 64 + dir);
		}

		signed char dx = 0, dy = 0;
		if (p.jy > 0)
		{
			dx = sintab16[dir];
			dy = sintab16[(dir + 4) & 15];
		}
		else if (p.jy < 0) 
		{
			dx = -sintab16[dir];
			dy = -sintab16[(dir + 4) & 15];
		}

		unsigned	tx = p.px + dx;
		unsigned	ty = p.py + dy;

		unsigned cx = (tx - 20 * 16) >> 7;
		unsigned cy = (ty - 48 * 16) >> 7;

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
	}
}

void view_sprite(char pi, char vi, int sx, int sy)
{
	if (sx > 0 && sy > 29 && sx < 168 && sy < 210)
		vspr_move(pi * 8 + vi, sx  + 22 * 8 * pi, sy);
	else
		vspr_hide(pi * 8 + vi);
}

void view_move(char pi)
{
	auto & p = players[pi];

	p.vx += p.dvx;
	p.vy += p.dvy;

	view_sprite(pi, 0, (players[0].px >> 4) - p.vx, (players[0].py >> 4) - p.vy);
	view_sprite(pi, 1, (players[1].px >> 4) - p.vx, (players[1].py >> 4) - p.vy);

	if ((players[0].px >> 4) + 16 - players[0].hqx < 32 && (players[0].py >> 4) + 16 - players[0].hqy < 32)
		vspr_hide(pi * 8 + 2);
	else
		view_sprite(pi, 2, players[0].hqx - p.vx, players[0].hqy - p.vy);

	if ((players[1].px >> 4) + 16 - players[1].hqx < 32 && (players[1].py >> 4) + 16 - players[1].hqy < 32)
		vspr_hide(pi * 8 + 3);
	else
		view_sprite(pi, 3, players[1].hqx - p.vx, players[1].hqy - p.vy);

	view_sprite(pi, 4, players[0].flagx - p.vx, players[0].flagy - p.vy);
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

void view_redraw(char pi, char phase)
{
	auto & p = players[pi];

	if (p.dvx || p.dvy)
	{
		font_expand(pi * 128, p.vx & 7, p.vy & 7);
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

int main(void)
{
	cia_init();

	// Install memory trampoline to ensure system
	// keeps running when playing with the PLA
	mmap_trampoline();

	// Swap in all RAM
	mmap_set(MMAP_RAM);

	oscar_expand_lzo(Charset, BackgroundFont);
	oscar_expand_lzo(Sprites, SpriteImages);

	// Swap ROM back in
	mmap_set(MMAP_NO_ROM);

	memset(Screen, 15, 1000);
	memset(Color, VCOL_MED_GREY, 1000);

	for(char y=0; y<20; y++)
	{
		for(char x=18; x<22; x++)
			Color[40 * y + x] = VCOL_DARK_GREY;
	}

	memset(Color + 40 * 20, VCOL_DARK_GREY, 40 * 5);

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

	vspr_sort();
	vspr_update();
	rirq_sort();

	rirq_start();

	player_init_hq(0, 32, 58);
	player_init_hq(1, 304, 330);
	player_init_flag(0, 64, 58);
	player_init_flag(1, 272, 330);

	player_init(0);
	player_init(1);

	char	phase = 0;
	while (true)
	{
		view_move(0);
		view_move(1);

		vspr_sort();

		rirq_wait();

		vspr_update();
		rirq_sort();

		view_redraw(0, phase & 7);
		view_redraw(1, (phase + 4) & 7);

		char flgani = 80 + ((phase >> 2) & 3); 

		vspr_image(4, flgani);
		vspr_image(5, flgani);
		vspr_image(12, flgani);
		vspr_image(13, flgani);

		phase++;

		player_check(0);
		player_check(1);


		player_ai(0);
		player_ai(1);

		player_move(0);
		player_move(1);
	}

	return 0;
}
