#include "gamemusic.h"
#include <c64/sid.h>


#pragma section( music, 0)

#pragma region( music, 0x9000, 0xc000, , , {music} )

#pragma data(music)

__export char music[] = {
	#embed 0x3000 0x7e "BattleTanks-7.2.sid" 
};

#pragma data(data)

char		music_throttle;

void music_init(Tune tune)
{
	__asm
	{
		lda		tune
		jsr		$9000
	}
}

void music_queue(Tune tune)
{
}

void music_fade(void)
{
	for(char i=0; i<15; i+=5)
	{
		sid.fmodevol = 15 - i;
		vic_waitFrame();
	}

}

void music_play(void)
{
	if (system_ntsc)
	{
		music_throttle++;
		if (music_throttle == 6)
		{
			music_throttle = 0;
			return;
		}
	}

	if (music_active)
	{
		__asm
		{
			jsr		$9003
		}
	}
}

void music_patch_voice3(bool enable)
{
	*(char *)0x916a = enable ? 0x20 : 0x4c;
}

void music_toggle(void)
{
}
