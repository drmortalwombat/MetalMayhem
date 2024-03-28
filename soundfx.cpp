#include "soundfx.h"
#include <audio/sidfx.h>

SIDFX	SIDFXExplosion[2] = {{
	2000, 0,
	SID_CTRL_GATE | SID_CTRL_SAW,
	SID_ATK_2 | SID_DKY_6,
	0xf0  | SID_DKY_750,
	-50, 0,
	2, 0,
	100
},{
	2000, 0,
	SID_CTRL_GATE | SID_CTRL_NOISE,
	SID_ATK_2 | SID_DKY_6,
	0xf0  | SID_DKY_2400,
	-10, 0,
	5, 200,
	50
}};

SIDFX	SIDFXShot[1] = {{
	8000, 0,
	SID_CTRL_GATE | SID_CTRL_NOISE,
	SID_ATK_2 | SID_DKY_6,
	0xf0  | SID_DKY_750,
	-100, 0,
	2, 20,
	10
}};

SIDFX	SIDFXFlagCapture[4] = {{
	NOTE_C(8), 3072, 
	SID_CTRL_GATE | SID_CTRL_SAW,
	SID_ATK_2 | SID_DKY_6,
	0xf0  | SID_DKY_6,
	0, 0,
	1, 0,
	4
},{
	NOTE_E(8), 3072, 
	SID_CTRL_GATE | SID_CTRL_SAW,
	SID_ATK_2 | SID_DKY_6,
	0xf0  | SID_DKY_6,
	0, 0,
	1, 0,
	4
},{
	NOTE_G(8), 3072, 
	SID_CTRL_GATE | SID_CTRL_SAW,
	SID_ATK_2 | SID_DKY_6,
	0xf0  | SID_DKY_6,
	0, 0,
	1, 0,
	4
},{
	NOTE_C(9), 3072, 
	SID_CTRL_GATE | SID_CTRL_SAW,
	SID_ATK_2 | SID_DKY_6,
	0xf0  | SID_DKY_300,
	0, 0,
	4, 32,
	4
}};

SIDFX	SIDFXFlagArrival[7] = {{
	NOTE_E(9), 1840, 
	SID_CTRL_GATE | SID_CTRL_TRI | SID_CTRL_RECT,
	SID_ATK_2 | SID_DKY_300,
	0x00  | SID_DKY_300,
	0, 0,
	1, 4,
	4
},{
	NOTE_G(9), 1840, 
	SID_CTRL_GATE | SID_CTRL_TRI | SID_CTRL_RECT,
	SID_ATK_2 | SID_DKY_300,
	0x00  | SID_DKY_300,
	0, 0,
	1, 1,
	4
},{
	NOTE_A(9), 1840, 
	SID_CTRL_GATE | SID_CTRL_TRI | SID_CTRL_RECT,
	SID_ATK_2 | SID_DKY_300,
	0x00  | SID_DKY_300,
	0, 0,
	1, 1,
	4
},{
	NOTE_B(9), 1840, 
	SID_CTRL_GATE | SID_CTRL_TRI | SID_CTRL_RECT,
	SID_ATK_2 | SID_DKY_300,
	0x00  | SID_DKY_300,
	0, 0,
	1, 4,
	4
},{
	NOTE_E(10), 1840, 
	SID_CTRL_GATE | SID_CTRL_TRI | SID_CTRL_RECT,
	SID_ATK_2 | SID_DKY_300,
	0x00  | SID_DKY_300,
	0, 0,
	1, 4,
	4
},{
	NOTE_D(10), 1840, 
	SID_CTRL_GATE | SID_CTRL_TRI | SID_CTRL_RECT,
	SID_ATK_2 | SID_DKY_300,
	0x00  | SID_DKY_300,
	0, 0,
	1, 4,
	4
},{
	NOTE_B(9), 1840, 
	SID_CTRL_GATE | SID_CTRL_TRI | SID_CTRL_RECT,
	SID_ATK_2 | SID_DKY_300,
	0x00  | SID_DKY_300,
	0, 0,
	1, 32,
	4
}};


SIDFX	SIDFXKatching[5] = {{
	NOTE_C(10), 512, 
	SID_CTRL_GATE | SID_CTRL_RECT,
	SID_ATK_2 | SID_DKY_114,
	0x80  | SID_DKY_114,
	0, 0,
	1, 0,
	2
},{
	NOTE_C(9), 512, 
	SID_CTRL_NOISE,
	SID_ATK_2 | SID_DKY_114,
	0x80  | SID_DKY_114,
	0, 0,
	1, 0,
	2
},{
	NOTE_G(10), 512, 
	SID_CTRL_GATE | SID_CTRL_RECT,
	SID_ATK_2 | SID_DKY_114,
	0x80  | SID_DKY_114,
	0, 0,
	1, 0,
	2
},{
	NOTE_G(9), 512, 
	SID_CTRL_NOISE,
	SID_ATK_2 | SID_DKY_114,
	0x80  | SID_DKY_114,
	0, 0,
	1, 0,
	2
},{
	NOTE_E(10), 512, 
	SID_CTRL_GATE | SID_CTRL_RECT,
	SID_ATK_2 | SID_DKY_750,
	0xf0  | SID_DKY_750,
	0, 16,
	4, 60,
	2
}};


void sfx_explosion(void)
{
	sidfx_play(2, SIDFXExplosion, 2);
}

void sfx_shot(void)
{
	sidfx_play(2, SIDFXShot, 1);
}

void sfx_flag_capture(void)
{
	sidfx_play(2, SIDFXFlagCapture, 4);
}

void sfx_flag_arrival(void)
{
	sidfx_play(2, SIDFXFlagArrival, 7);
}

void sfx_katching(void)
{
	sidfx_play(2, SIDFXKatching, 5);
}

